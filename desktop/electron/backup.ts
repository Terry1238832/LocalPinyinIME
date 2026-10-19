// Export and import of the user's data, part by part. A backup is one
// gzip-compressed JSON file (.lpbackup) holding each chosen part's files.
// Importing merges a part into what is there or replaces it; files the input
// method reads go into place through the settings bridge, under the locks the
// input method writes them with.
import { randomUUID } from 'node:crypto';
import { mkdir, readdir, readFile, rm, stat, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { promisify } from 'node:util';
import { gunzip, gzip } from 'node:zlib';
import { backupParts, type BackupMode, type BackupPart, type BackupSummary } from '../src/types';
import type { Bridge } from './panel';

const FORMAT = 'localpinyin-backup';
const MAX_BYTES = 600 * 1024 * 1024;
const zip = promisify(gzip);
const unzip = promisify(gunzip);

type Context = { dataDirectory: string; userData: string; bridge: Bridge; version: string; reloadPanel: () => void };
type Files = Record<string, Buffer>;
type Bundle = { format: string; version: number; app: string; created: string; parts: Partial<Record<BackupPart, Record<string, string>>> };

// The file names each part may hold.
const NAMES: Record<BackupPart, RegExp> = {
  settings: /^settings\.json$/,
  lexicon: /^user_lexicon\.tsv$/,
  learning: /^user_learning\.tsv$/,
  phrases: /^(custom_phrases|emoji_candidates)\.tsv$/,
  panel: /^panel\.json$/,
  stickers: /^(index\.json|[0-9a-f-]{36}\.(png|jpe?g|gif|webp|bmp))$/,
  clipboard: /^(history\.jsonl|[0-9a-f]{16}\.(png|txt))$/,
  records: /^vault\.json$/,
};

// Where each part lives; single-file parts list their files.
function location(context: Context, part: BackupPart): { directory: string; names: RegExp; listed?: string[] } {
  const data = context.dataDirectory, names = NAMES[part];
  switch (part) {
    case 'settings': return { directory: data, names, listed: ['settings.json'] };
    case 'lexicon': return { directory: data, names, listed: ['user_lexicon.tsv'] };
    case 'learning': return { directory: data, names, listed: ['user_learning.tsv'] };
    case 'phrases': return { directory: data, names, listed: ['custom_phrases.tsv', 'emoji_candidates.tsv'] };
    case 'panel': return { directory: context.userData, names, listed: ['panel.json'] };
    case 'stickers': return { directory: path.join(data, 'stickers'), names };
    case 'records': return { directory: data, names, listed: ['vault.json'] };
    case 'clipboard': return { directory: path.join(data, 'clipboard'), names };
  }
}

async function readPart(context: Context, part: BackupPart): Promise<Files> {
  const { directory, names, listed } = location(context, part);
  let candidates = listed;
  if (!candidates) { try { candidates = await readdir(directory); } catch { candidates = []; } }
  const files: Files = {};
  for (const name of candidates) {
    if (!names.test(name)) continue;
    try { files[name] = await readFile(path.join(directory, name)); } catch { /* not there */ }
  }
  return files;
}

function count(part: BackupPart, files: Files) {
  const lines = (name: string) => files[name] ? files[name].toString('utf8').split('\n').filter(line => line.trim() && !line.startsWith('#')).length : 0;
  switch (part) {
    case 'settings': case 'panel': return Object.keys(files).length;
    case 'lexicon': return lines('user_lexicon.tsv');
    case 'learning': return lines('user_learning.tsv');
    case 'phrases': return lines('custom_phrases.tsv') + lines('emoji_candidates.tsv');
    case 'stickers': return Object.keys(files).filter(name => name !== 'index.json').length;
    case 'clipboard': return lines('history.jsonl');
    case 'records': return Object.keys(files).length;
  }
}

function summary(file: string, bundle: Bundle, parts: Partial<Record<BackupPart, Files>>): BackupSummary {
  const result: BackupSummary = { file, created: bundle.created, version: bundle.app, parts: {} };
  for (const part of backupParts) {
    const files = parts[part];
    if (!files) continue;
    result.parts[part] = { files: Object.keys(files).length, items: count(part, files),
      bytes: Object.values(files).reduce((total, data) => total + data.length, 0) };
  }
  return result;
}

function partList(value: unknown): BackupPart[] {
  if (!Array.isArray(value) || !value.length || value.some(part => !(backupParts as readonly string[]).includes(part)))
    throw new Error('请选择要导出的内容。');
  return backupParts.filter(part => value.includes(part));
}

export async function exportBackup(context: Context, value: unknown, file: string): Promise<BackupSummary> {
  const parts = partList(value);
  const bundle: Bundle = { format: FORMAT, version: 1, app: context.version, created: new Date().toISOString(), parts: {} };
  const read: Partial<Record<BackupPart, Files>> = {};
  let bytes = 0;
  for (const part of parts) {
    const files = await readPart(context, part);
    read[part] = files;
    bundle.parts[part] = {};
    for (const [name, data] of Object.entries(files)) {
      bytes += data.length;
      if (bytes > MAX_BYTES) throw new Error('数据超过 600 MB，请少选几项（例如剪贴板里的图片）分开导出。');
      bundle.parts[part]![name] = data.toString('base64');
    }
  }
  await writeFile(file, await zip(Buffer.from(JSON.stringify(bundle), 'utf8')));
  return summary(file, bundle, read);
}

async function load(file: string): Promise<{ bundle: Bundle; parts: Partial<Record<BackupPart, Files>> }> {
  const info = await stat(file);
  if (!info.isFile() || info.size > MAX_BYTES) throw new Error('备份文件无效。');
  let bundle: Bundle;
  try { bundle = JSON.parse((await unzip(await readFile(file))).toString('utf8')) as Bundle; }
  catch { throw new Error('这不是本地拼音的备份文件，或文件已损坏。'); }
  if (!bundle || bundle.format !== FORMAT || bundle.version !== 1 || typeof bundle.parts !== 'object')
    throw new Error('这不是本地拼音的备份文件，或版本不受支持。');
  const parts: Partial<Record<BackupPart, Files>> = {};
  for (const part of backupParts) {
    const stored = bundle.parts[part];
    if (!stored || typeof stored !== 'object') continue;
    const names = NAMES[part];
    const files: Files = {};
    for (const [name, data] of Object.entries(stored)) {
      if (!names.test(name) || typeof data !== 'string') throw new Error('备份文件里有无法识别的内容。');
      files[name] = Buffer.from(data, 'base64');
    }
    parts[part] = files;
  }
  return { bundle, parts };
}

export async function inspectBackup(file: string): Promise<BackupSummary> {
  const { bundle, parts } = await load(file);
  return summary(file, bundle, parts);
}

// Rows keyed by their first columns; `combine` picks between a current row
// and an imported one with the same key.
function mergeRows(current: Buffer | undefined, imported: Buffer | undefined, keyColumns: number,
                   combine: (mine: string[], theirs: string[]) => string[]): Buffer {
  const rows = new Map<string, string[]>();
  const header: string[] = [];
  const add = (data: Buffer | undefined, mine: boolean) => {
    for (const line of (data?.toString('utf8') ?? '').split(/\r?\n/)) {
      if (!line.trim()) continue;
      if (line.startsWith('#')) { if (mine || !header.length) header.push(line); continue; }
      const columns = line.split('\t');
      const key = columns.slice(0, keyColumns).join('\t');
      const existing = rows.get(key);
      rows.set(key, existing ? combine(existing, columns) : columns);
    }
  };
  add(current, true);
  add(imported, false);
  return Buffer.from([...new Set(header), ...[...rows.values()].map(columns => columns.join('\t'))].join('\n') + '\n', 'utf8');
}

const larger = (a: string | undefined, b: string | undefined) => String(Math.max(Number(a) || 0, Number(b) || 0));

function mergeObjects(current: unknown, imported: unknown): unknown {
  if (!current || typeof current !== 'object' || Array.isArray(current) || !imported || typeof imported !== 'object' || Array.isArray(imported))
    return imported ?? current;
  const result: Record<string, unknown> = { ...(current as Record<string, unknown>) };
  for (const [key, value] of Object.entries(imported as Record<string, unknown>)) result[key] = mergeObjects(result[key], value);
  return result;
}

function mergePanel(current: Buffer | undefined, imported: Buffer | undefined): Buffer {
  type State = { favorites?: Record<string, string[]>; recents?: Record<string, string[]> };
  const parse = (data: Buffer | undefined): State => { try { return data ? JSON.parse(data.toString('utf8')) : {}; } catch { return {}; } };
  const mine = parse(current), theirs = parse(imported);
  const result: State = { favorites: {}, recents: {} };
  for (const field of ['favorites', 'recents'] as const) {
    for (const kind of ['symbol', 'kaomoji', 'emoji']) {
      const merged = [...(mine[field]?.[kind] ?? []), ...(theirs[field]?.[kind] ?? [])].filter(item => typeof item === 'string');
      result[field]![kind] = [...new Set(merged)].slice(0, field === 'recents' ? 48 : 500);
    }
  }
  return Buffer.from(JSON.stringify(result, null, 2) + '\n', 'utf8');
}

export async function importBackup(context: Context, file: string, value: unknown): Promise<{ imported: BackupPart[] }> {
  const choices = (value && typeof value === 'object' ? value : {}) as Partial<Record<BackupPart, BackupMode>>;
  const { parts } = await load(file);
  const chosen = backupParts.filter(part => parts[part] && (choices[part] === 'merge' || choices[part] === 'overwrite'));
  if (!chosen.length) throw new Error('请选择要导入的内容。');
  // Short names: the data folder may already be deep.
  const staging = path.join(context.dataDirectory, `import-${randomUUID().slice(0, 8)}`);
  await mkdir(staging, { recursive: true });
  // Into place through the bridge: it takes the file's lock and tells the
  // input method to read it again.
  const place = async (kind: string, data: Buffer) => {
    const temporary = path.join(staging, `${kind}-${randomUUID().slice(0, 8)}.tmp`);
    await writeFile(temporary, data);
    await context.bridge('replace-data', [kind, temporary]);
  };
  try {
    for (const part of chosen) {
      const imported = parts[part]!;
      const merge = choices[part] === 'merge';
      const current = merge ? await readPart(context, part) : {};
      if (part === 'settings') {
        const parse = (data?: Buffer) => { try { return data ? JSON.parse(data.toString('utf8')) : {}; } catch { return {}; } };
        const merged = merge ? mergeObjects(parse(current['settings.json']), parse(imported['settings.json'])) : parse(imported['settings.json']);
        await place('settings', Buffer.from(JSON.stringify(merged, null, 2) + '\n', 'utf8'));
      } else if (part === 'lexicon') {
        const name = 'user_lexicon.tsv';
        await place('lexicon', merge ? mergeRows(current[name], imported[name], 2,
          (mine, theirs) => [mine[0], mine[1], larger(mine[2], theirs[2])]) : imported[name] ?? Buffer.alloc(0));
      } else if (part === 'learning') {
        const name = 'user_learning.tsv';
        await place('learning', merge ? mergeRows(current[name], imported[name], 2,
          (mine, theirs) => [mine[0], mine[1], larger(mine[2], theirs[2]), larger(mine[3], theirs[3]), larger(mine[4], theirs[4])])
          : imported[name] ?? Buffer.alloc(0));
      } else if (part === 'phrases') {
        for (const [kind, name] of [['phrases', 'custom_phrases.tsv'], ['emoji', 'emoji_candidates.tsv']] as const) {
          if (!merge && !imported[name]) { await place(kind, Buffer.alloc(0)); continue; }
          await place(kind, merge ? mergeRows(current[name], imported[name], 2, mine => mine) : imported[name]);
        }
      } else if (part === 'panel') {
        const name = 'panel.json';
        await mkdir(context.userData, { recursive: true });
        await writeFile(path.join(context.userData, name), merge ? mergePanel(current[name], imported[name]) : imported[name] ?? Buffer.from('{}\n'));
        context.reloadPanel();
      } else if (part === 'stickers') {
        const directory = location(context, part).directory;
        type Sticker = { id: string; ext: string; name: string; added: number };
        const parse = (data?: Buffer): Sticker[] => { try { const list = data ? JSON.parse(data.toString('utf8')) : []; return Array.isArray(list) ? list : []; } catch { return []; } };
        const theirs = parse(imported['index.json']).filter(item => typeof item?.id === 'string' && imported[item.id + item.ext]);
        if (!merge) await rm(directory, { recursive: true, force: true });
        await mkdir(directory, { recursive: true });
        const mine = merge ? parse(current['index.json']) : [];
        const known = new Set(mine.map(item => item.id));
        const added = theirs.filter(item => !known.has(item.id));
        for (const item of added) await writeFile(path.join(directory, item.id + item.ext), imported[item.id + item.ext]);
        await writeFile(path.join(directory, 'index.json'), JSON.stringify([...mine, ...added], null, 2) + '\n');
      } else if (part === 'records') {
        // Encrypted as a whole: it replaces what is here (merging would need both passwords).
        if (imported['vault.json']) await place('vault', imported['vault.json']);
      } else if (part === 'clipboard') {
        const directory = location(context, part).directory;
        if (!merge) await context.bridge('clipboard-clear');
        await mkdir(directory, { recursive: true });
        for (const [name, data] of Object.entries(imported)) if (name !== 'history.jsonl') await writeFile(path.join(directory, name), data);
        // One JSON object per line; a copy in both keeps the one here.
        const byId = new Map<string, string>();
        const lines = [...(merge ? current['history.jsonl']?.toString('utf8') ?? '' : '').split('\n'),
          ...(imported['history.jsonl']?.toString('utf8') ?? '').split('\n')];
        for (const line of lines) {
          try { const item = JSON.parse(line) as { id?: unknown }; if (typeof item.id === 'string' && !byId.has(item.id)) byId.set(item.id, line); } catch { /* skip */ }
        }
        await place('clipboard', Buffer.from([...byId.values()].join('\n') + (byId.size ? '\n' : ''), 'utf8'));
      }
    }
  } finally {
    await rm(staging, { recursive: true, force: true });
  }
  return { imported: chosen };
}
