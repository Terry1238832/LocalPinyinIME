// The clipboard history as the panel sees it. LocalPinyinClipboard.exe
// records copies into <data>/clipboard (history.jsonl, a PNG per picture, a
// text file per long text) and reads the words in pictures; this side only
// reads that folder, and asks the settings bridge to paste or delete under
// the lock the recorder writes with.
import { readFile } from 'node:fs/promises';
import path from 'node:path';
import type { ClipItem, ClipList, ClipQuery } from '../src/panel/types';
import type { Settings } from '../src/types';
import type { Bridge, Panel } from './panel';

const ID = /^[0-9a-f]{16}$/;
const LIST_LIMIT = 400;

export function clipboardFile(dataDirectory: string, name: string): string | null {
  if (!/^[0-9a-f]{16}\.png$/.test(name)) return null;
  return path.join(dataDirectory, 'clipboard', name);
}

export async function readClipboardHistory(dataDirectory: string): Promise<ClipItem[]> {
  let raw: string;
  try { raw = await readFile(path.join(dataDirectory, 'clipboard', 'history.jsonl'), 'utf8'); } catch { return []; }
  const items: ClipItem[] = [];
  for (const line of raw.split('\n')) {
    if (!line.trim()) continue;
    try {
      const item = JSON.parse(line) as ClipItem;
      if (typeof item.id !== 'string' || !ID.test(item.id) || (item.kind !== 'text' && item.kind !== 'image') ||
          !Number.isFinite(item.time)) continue;
      items.push(item);
    } catch { /* a line being written */ }
  }
  return items.sort((a, b) => b.time - a.time);
}

function localDay(time: number) {
  const date = new Date(time);
  return `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${String(date.getDate()).padStart(2, '0')}`;
}

export function installClipboardHandlers(options: {
  panel: Panel; dataDirectory: () => string; bridge: Bridge; settings: () => Promise<Settings>;
  saveSettings: (patch: Partial<Settings>) => Promise<Settings>;
}) {
  const { panel, bridge } = options;
  const id = (value: unknown) => {
    if (typeof value !== 'string' || !ID.test(value)) throw new Error('剪贴板记录不存在。');
    return value;
  };
  panel.handle('clipboard', async (value): Promise<ClipList> => {
    const query = (value && typeof value === 'object' ? value : {}) as ClipQuery;
    const settings = await options.settings();
    const now = Date.now();
    const keep = settings.clipboardRetentionDays;
    const all = (await readClipboardHistory(options.dataDirectory()))
      .filter(item => !keep || now - item.time <= keep * 24 * 3600 * 1000);
    const counts = new Map<string, number>();
    for (const item of all) { const day = localDay(item.time); counts.set(day, (counts.get(day) ?? 0) + 1); }
    const search = typeof query.search === 'string' ? query.search.trim().toLowerCase().slice(0, 100) : '';
    const day = typeof query.day === 'string' && /^\d{4}-\d{2}-\d{2}$/.test(query.day) ? query.day : '';
    const matched = all.filter(item => (!day || localDay(item.time) === day) &&
      (!search || `${item.text ?? ''}\n${item.ocr ?? ''}`.toLowerCase().includes(search)));
    const limit = Number.isInteger(query.limit) && (query.limit as number) > 0 ? Math.min(query.limit as number, LIST_LIMIT) : 120;
    return { status: settings.clipboardHistory, retentionDays: keep, items: matched.slice(0, limit), total: matched.length,
      days: [...counts].map(([key, count]) => ({ day: key, count })).slice(0, 400) };
  });
  panel.handle('clipboard-text', async (value) => {
    const key = id(value);
    const item = (await readClipboardHistory(options.dataDirectory())).find(entry => entry.id === key);
    if (!item || item.kind !== 'text') throw new Error('剪贴板记录不存在。');
    if (!item.long) return item.text ?? '';
    return readFile(path.join(options.dataDirectory(), 'clipboard', key + '.txt'), 'utf8');
  });
  panel.handle('clipboard-paste', async (value) => {
    const key = id(value);
    const host = panel.host;
    if (!host) throw new Error('没有找到要输入的位置。');
    await bridge('clipboard-paste', [host, key]);
    panel.stepAside();
  });
  panel.handle('clipboard-delete', (value) => bridge('clipboard-delete', [id(value)]));
  panel.handle('clipboard-consent', async (on) => {
    if (typeof on !== 'boolean') throw new Error('参数无效。');
    await options.saveSettings({ clipboardHistory: on ? 'on' : 'off' });
  });
}
