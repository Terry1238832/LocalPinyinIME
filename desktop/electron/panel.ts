// The tool panel: a second window of the settings app that the input method
// opens when Tab is pressed twice. It lists symbols, kaomoji, emoji, the
// user's stickers, the clipboard history and the typing tools; what the user
// confirms goes back to the input method in the application it came from.
import { BrowserWindow, ipcMain, nativeTheme, screen, session, type IpcMainInvokeEvent, type Session } from 'electron';
import { mkdir, readFile, rm, stat, writeFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { randomUUID } from 'node:crypto';
import os from 'node:os';
import path from 'node:path';
import { panelPages, type PanelData, type PanelKind, type PanelPageId, type PanelState, type Sticker } from '../src/panel/types';
import type { Settings } from '../src/types';

export type PanelTarget = { ime: string; host: string; token: string; anchor: { x: number; y: number; bottom: number };
  page: PanelPageId | null };

export type Bridge = <T>(command: string, args?: string[]) => Promise<T>;

const PANEL_WIDTH = 780;
const PANEL_HEIGHT = 480;
// Without "keep ready", the app leaves this long after the panel was last used.
const IDLE_EXIT_MS = 10 * 60 * 1000;
const RECENT_LIMIT = 48;
const STICKER_TYPES: Record<string, string> = { '.png': 'image/png', '.jpg': 'image/jpeg', '.jpeg': 'image/jpeg',
  '.gif': 'image/gif', '.webp': 'image/webp', '.bmp': 'image/bmp' };
const STICKER_MAX_BYTES = 20 * 1024 * 1024;
// Pictures are known by their first bytes, not their names: a picture from
// a chat program's cache may have none, or end in .dat.
const SIGNATURES: [string, number, number[]][] = [['.png', 0, [0x89, 0x50, 0x4e, 0x47]], ['.jpg', 0, [0xff, 0xd8, 0xff]],
  ['.gif', 0, [0x47, 0x49, 0x46, 0x38]], ['.webp', 8, [0x57, 0x45, 0x42, 0x50]], ['.bmp', 0, [0x42, 0x4d]]];

function pictureType(data: Uint8Array): string | null {
  for (const [ext, at, bytes] of SIGNATURES)
    if (data.length > at + bytes.length && bytes.every((byte, index) => data[at + index] === byte) &&
        (ext !== '.webp' || (data[0] === 0x52 && data[1] === 0x49))) return ext;
  return null;
}

// A picture copied or dragged from a web page arrives as its first frame;
// its address gives the original (a GIF keeps moving). Fetched only when the
// user adds it, in a session of its own that keeps no cookies or cache and
// reaches nothing but http(s); the Referer is the picture's own site, which
// sites that refuse pictures shown elsewhere accept.
let pictureNetwork: Session | null = null;
function pictureSession() {
  if (pictureNetwork) return pictureNetwork;
  const network = session.fromPartition('localpinyin-pictures', { cache: false });
  network.webRequest.onBeforeRequest((details, callback) => callback({ cancel: !/^https?:\/\//i.test(details.url) }));
  network.webRequest.onBeforeSendHeaders((details, callback) => {
    callback({ requestHeaders: { ...details.requestHeaders, Referer: new URL(details.url).origin + '/' } });
  });
  network.setPermissionRequestHandler((_contents, _permission, callback) => callback(false));
  pictureNetwork = network;
  return network;
}

async function downloadPicture(link: string): Promise<{ name: string; data: Uint8Array }> {
  let url: URL;
  try { url = new URL(link); } catch { throw new Error('图片地址无效。'); }
  if (url.protocol !== 'https:' && url.protocol !== 'http:') throw new Error('图片地址无效。');
  const response = await pictureSession().fetch(url.href, { headers: { Accept: 'image/*,*/*;q=0.5' }, signal: AbortSignal.timeout(20_000) })
    .catch(() => { throw new Error('没能连上图片所在的网站。'); });
  if (!response.ok || !response.body) throw new Error(`没能取到原图（网站返回 ${response.status}）。`);
  if (Number(response.headers.get('content-length') ?? 0) > STICKER_MAX_BYTES) throw new Error('图片太大（最大 20 MB）。');
  const reader = response.body.getReader();
  const chunks: Uint8Array[] = [];
  let size = 0;
  for (;;) {
    const { done, value } = await reader.read();
    if (done) break;
    size += value.byteLength;
    if (size > STICKER_MAX_BYTES) { await reader.cancel(); throw new Error('图片太大（最大 20 MB）。'); }
    chunks.push(value);
  }
  let name = '';
  try { name = decodeURIComponent(path.posix.basename(url.pathname)); } catch { /* keep it unnamed */ }
  return { name: name || '网上的图片', data: Buffer.concat(chunks) };
}

// WeChat keeps received pictures as .dat files: the picture with every byte
// XORed with one key. Try the key each picture type's first byte gives.
function unscramble(data: Uint8Array): Uint8Array | null {
  if (data.length < 16) return null;
  for (const [, at, bytes] of SIGNATURES) {
    if (at !== 0) continue;
    const key = data[0] ^ bytes[0];
    if (!key || bytes.some((byte, index) => (data[index] ^ key) !== byte)) continue;
    const plain = data.map(byte => byte ^ key);
    if (pictureType(plain)) return plain;
  }
  return null;
}

// The input method in a store app's sandbox cannot start programs; Windows
// starts this app for it through its link instead: localpinyin:settings, or
// localpinyin:panel?ime=…&host=…&token=…&anchor=…&page=…, read here as the
// arguments it stands for.
export function linkArguments(argv: string[]): string[] {
  const link = argv.find(item => /^localpinyin:/i.test(item));
  if (!link) return argv;
  const rest = argv.filter(item => item !== link);
  try {
    const url = new URL(link);
    if (url.pathname.replace(/^\/+/, '').toLowerCase() !== 'panel') return rest;
    return [...rest, '--panel', ...[...url.searchParams].map(([name, value]) => `--${name}=${value}`)];
  } catch { return rest; }
}

// --panel --ime=<window> --host=<window> --token=<hex> --anchor=<x>,<y>,<bottom>
export function panelArguments(argv: string[]): PanelTarget | null {
  if (!argv.includes('--panel')) return null;
  const value = (name: string) => argv.find(item => item.startsWith(`--${name}=`))?.slice(name.length + 3) ?? '';
  const ime = value('ime'), host = value('host'), token = value('token');
  const anchor = value('anchor').split(',').map(Number);
  if (!/^\d{1,20}$/.test(ime) || !/^\d{1,20}$/.test(host) || !/^[0-9a-f]{32}$/.test(token) ||
      anchor.length !== 3 || anchor.some(number => !Number.isFinite(number))) return null;
  const page = value('page');
  return { ime, host, token, anchor: { x: anchor[0], y: anchor[1], bottom: anchor[2] },
    page: (panelPages as readonly string[]).includes(page) ? page as PanelPageId : null };
}

export class Panel {
  window: BrowserWindow | null = null;
  private target: PanelTarget | null = null;
  private idle: NodeJS.Timeout | null = null;
  // Pinned, or on the 表情包 page: stays after sending something.
  private keepOpen = false;
  private state: PanelState = { favorites: { symbol: [], kaomoji: [], emoji: [] }, recents: { symbol: [], kaomoji: [], emoji: [] } };
  private loaded = false;

  constructor(private readonly options: {
    dataDirectory: string; userData: string; bridge: Bridge; settings: () => Promise<Settings>;
    preload: string; icon: string; hidden: boolean; settingsOpen: () => boolean; openSettings: () => void; quit: () => void;
  }) {}

  // Whether the app stays when its other windows close: the panel was used
  // in the last ten minutes, or waits in the background by choice.
  keepsApp() { return this.resident || (!!this.window && (this.window.isVisible() || !!this.idle)); }
  resident = false;

  private get stateFile() { return path.join(this.options.userData, 'panel.json'); }
  private get stickerDirectory() { return path.join(this.options.dataDirectory, 'stickers'); }

  // Imported favorites and recents: read the file again when next needed.
  reloadState() { this.loaded = false; }

  private async loadState() {
    if (this.loaded) return;
    this.loaded = true;
    try {
      const saved = JSON.parse(await readFile(this.stateFile, 'utf8')) as Partial<PanelState>;
      for (const kind of ['symbol', 'kaomoji', 'emoji'] as PanelKind[]) {
        const strings = (value: unknown) => Array.isArray(value) ? value.filter(item => typeof item === 'string' && item.length <= 64).slice(0, 500) : [];
        this.state.favorites[kind] = strings(saved.favorites?.[kind]);
        this.state.recents[kind] = strings(saved.recents?.[kind]).slice(0, RECENT_LIMIT);
      }
    } catch { /* first use */ }
  }

  private async saveState() {
    await mkdir(path.dirname(this.stateFile), { recursive: true });
    await writeFile(this.stateFile, JSON.stringify(this.state, null, 2) + '\n', 'utf8');
  }

  async stickers(): Promise<Sticker[]> {
    try {
      const list = JSON.parse(await readFile(path.join(this.stickerDirectory, 'index.json'), 'utf8')) as Sticker[];
      return Array.isArray(list) ? list.filter(item => typeof item?.id === 'string' && /^[0-9a-f-]{36}$/.test(item.id) &&
        typeof item.ext === 'string' && item.ext in STICKER_TYPES) : [];
    } catch { return []; }
  }

  private async writeStickers(list: Sticker[]) {
    await mkdir(this.stickerDirectory, { recursive: true });
    await writeFile(path.join(this.stickerDirectory, 'index.json'), JSON.stringify(list, null, 2) + '\n', 'utf8');
    // Readable by store apps too, where the small panel shows them.
    void this.options.bridge('share-stickers').catch(() => undefined);
  }

  // A sticker file for the app:// protocol, or null.
  async stickerFile(name: string): Promise<{ file: string; type: string } | null> {
    const match = /^([0-9a-f-]{36})(\.[a-z]+)$/.exec(name);
    if (!match || !(match[2] in STICKER_TYPES)) return null;
    const list = await this.stickers();
    if (!list.some(item => item.id === match[1] && item.ext === match[2])) return null;
    return { file: path.join(this.stickerDirectory, name), type: STICKER_TYPES[match[2]] };
  }

  async open(target: PanelTarget) {
    this.target = target;
    // The small panel may have changed favorites and recents since.
    this.loaded = false;
    await this.loadState();
    if (this.idle) { clearTimeout(this.idle); this.idle = null; }
    const settings = await this.options.settings().catch(() => null);
    if (settings) { nativeTheme.themeSource = settings.theme; this.resident = settings.panelResident; }
    const window = this.window ?? this.create();
    this.place(window, target);
    window.webContents.send('localpinyin:panel-open');
    if (this.options.hidden) return;
    if (window.webContents.isLoading()) window.once('ready-to-show', () => { window.show(); window.focus(); });
    else { window.show(); window.focus(); }
  }

  // After sending something: put away unless pinned (or dragging pictures in).
  stepAside() { if (!this.keepOpen) this.hide(); }

  hide() {
    if (!this.window || !this.window.isVisible()) return;
    this.window.hide();
    this.scheduleIdleExit();
  }

  private scheduleIdleExit() {
    if (this.idle) clearTimeout(this.idle);
    this.idle = setTimeout(() => {
      this.idle = null;
      void this.options.settings().then(settings => {
        this.resident = settings.panelResident;
        if (settings.panelResident || this.window?.isVisible()) return;
        this.window?.destroy();
        this.window = null;
        if (!this.options.settingsOpen()) this.options.quit();
      }).catch(() => undefined);
    }, IDLE_EXIT_MS);
  }

  // The settings window closed: leave now unless the panel keeps the app.
  async settingsClosed() {
    const settings = await this.options.settings().catch(() => null);
    this.resident = settings?.panelResident ?? false;
    if (this.resident) { if (!this.window) this.create(); return; }
    if (!this.window) { if (!this.options.settingsOpen()) this.options.quit(); return; }
    if (!this.window.isVisible() && !this.idle) this.scheduleIdleExit();
  }

  private create() {
    const window = new BrowserWindow({ width: PANEL_WIDTH, height: PANEL_HEIGHT, frame: false, resizable: false,
      maximizable: false, minimizable: false, fullscreenable: false, skipTaskbar: true, alwaysOnTop: true, show: false,
      backgroundColor: nativeTheme.shouldUseDarkColors ? '#16181c' : '#f6f7f9', title: '本地拼音 · 工具面板', icon: this.options.icon,
      webPreferences: { preload: this.options.preload, contextIsolation: true, sandbox: true, nodeIntegration: false,
        webSecurity: true, spellcheck: false, devTools: false, backgroundThrottling: !this.options.hidden, offscreen: this.options.hidden } });
    window.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
    window.webContents.on('will-navigate', event => event.preventDefault());
    window.webContents.on('will-attach-webview', event => event.preventDefault());
    // Unlike the input method's small panel, this window stays when something
    // else is clicked (a picture can be dragged in from a chat or a folder);
    // × or Esc puts it away.
    window.on('closed', () => { if (this.window === window) this.window = null; });
    void window.loadURL('app://local/panel.html');
    this.window = window;
    return window;
  }

  // Below the caret, or above it when there is no room; inside the screen.
  private place(window: BrowserWindow, target: PanelTarget) {
    const caret = screen.screenToDipPoint({ x: target.anchor.x, y: target.anchor.y });
    const bottom = screen.screenToDipPoint({ x: target.anchor.x, y: target.anchor.bottom });
    const area = screen.getDisplayNearestPoint(caret).workArea;
    let x = Math.round(caret.x - 40);
    let y = Math.round(bottom.y + 10);
    if (y + PANEL_HEIGHT > area.y + area.height) y = Math.round(caret.y - 10 - PANEL_HEIGHT);
    x = Math.min(Math.max(x, area.x + 8), area.x + area.width - PANEL_WIDTH - 8);
    y = Math.min(Math.max(y, area.y + 8), area.y + area.height - PANEL_HEIGHT - 8);
    window.setBounds({ x, y, width: PANEL_WIDTH, height: PANEL_HEIGHT });
  }

  // The latest picks first in each kind's 最近使用.
  private remember(picks: { kind: PanelKind; value: string }[]) {
    for (const kind of ['symbol', 'kaomoji', 'emoji'] as PanelKind[]) {
      const values = [...new Set(picks.filter(pick => pick.kind === kind).map(pick => pick.value).reverse())];
      if (!values.length) continue;
      const recent = this.state.recents[kind].filter(item => !values.includes(item));
      this.state.recents[kind] = [...values, ...recent].slice(0, RECENT_LIMIT);
    }
    if (picks.length) void this.saveState().catch(() => undefined);
  }

  // An IPC handler that only the panel's page may call.
  handle(channel: string, callback: (...args: unknown[]) => unknown) {
    ipcMain.handle('localpinyin:panel:' + channel, (event: IpcMainInvokeEvent, ...args) => {
      if (!this.window || event.sender !== this.window.webContents || event.senderFrame?.url !== 'app://local/panel.html')
        throw new Error('请求来源无效。');
      return callback(...args);
    });
  }

  // The application the panel was opened from, for pasting into it.
  get host() { return this.target?.host ?? null; }

  install() {
    const handle = this.handle.bind(this);
    const text = (value: unknown, maximum: number) => {
      if (typeof value !== 'string' || !value || value.length > maximum || /[\0\r\n]/.test(value)) throw new Error('内容无效。');
      return value;
    };
    const kindOf = (value: unknown): PanelKind | undefined =>
      value === 'symbol' || value === 'kaomoji' || value === 'emoji' ? value : undefined;
    handle('data', async (): Promise<PanelData> => {
      await this.loadState();
      return { state: this.state, stickers: await this.stickers(), dark: nativeTheme.shouldUseDarkColors, page: this.target?.page ?? null };
    });
    handle('close', () => this.hide());
    handle('insert', async (value, picks) => {
      const insertion = text(value, 4096);
      if (!this.target) throw new Error('没有找到要输入的位置。');
      const target = this.target;
      const list = Array.isArray(picks) ? picks.slice(0, 64).flatMap(pick => {
        const kind = kindOf((pick as { kind?: unknown })?.kind), item = (pick as { value?: unknown })?.value;
        return kind && typeof item === 'string' && item && item.length <= 64 ? [{ kind, value: item }] : [];
      }) : [];
      this.remember(list);
      // While the panel is still in front: Windows lets a process started by
      // the front one bring the application back; then the panel steps aside.
      await this.options.bridge('panel-insert', [target.ime, target.host, target.token, insertion]);
      this.stepAside();
    });
    handle('paste', async (id) => {
      if (!this.target) throw new Error('没有找到要输入的位置。');
      const list = await this.stickers();
      const sticker = list.find(item => item.id === id);
      if (!sticker) throw new Error('表情包不存在。');
      const target = this.target;
      await this.options.bridge('panel-paste', [target.host, path.join(this.stickerDirectory, sticker.id + sticker.ext)]);
      this.stepAside();
    });
    handle('favorite', async (kind, value, on) => {
      const which = kindOf(kind);
      if (!which || typeof on !== 'boolean') throw new Error('参数无效。');
      const item = text(value, 64);
      const list = this.state.favorites[which].filter(entry => entry !== item);
      this.state.favorites[which] = on ? [item, ...list].slice(0, 500) : list;
      await this.saveState();
      return this.state;
    });
    handle('tools', (value) => this.options.bridge('tools', [text(value, 128)]));
    handle('phrases', () => this.options.bridge('phrases-list'));
    handle('phrase-add', async (value, labels) => {
      const content = text(value, 64);
      if (!Array.isArray(labels) || !labels.length || labels.length > 10) throw new Error('请至少写一个短语。');
      let result: unknown = null;
      for (const label of labels) result = await this.options.bridge('phrases-add', [content, text(label, 32)]);
      return result;
    });
    handle('learn-emoji', async (value, names) => {
      const emoji = text(value, 32);
      if (!Array.isArray(names)) throw new Error('参数无效。');
      const labels = names.filter(name => typeof name === 'string' && /^[㐀-鿿]{1,8}$/.test(name)).slice(0, 4) as string[];
      if (labels.length) await this.options.bridge('emoji-learn', [emoji, ...labels]).catch(() => undefined);
    });
    handle('phrase-delete', (pinyin, value) => this.options.bridge('phrases-delete', [text(pinyin, 64), text(value, 64)]));
    const addStickers = async (files: { name: string; data: Uint8Array }[]) => {
      if (!files.length || files.length > 50) throw new Error('一次最多添加 50 张。');
      const list = await this.stickers();
      await mkdir(this.stickerDirectory, { recursive: true });
      for (const { name, data } of files) {
        if (!data.byteLength || data.byteLength > STICKER_MAX_BYTES) throw new Error(`图片太大：${name || '未命名'}（最大 20 MB）`);
        const picture = pictureType(data) ? data : unscramble(data);
        const ext = picture && pictureType(picture);
        if (!picture || !ext) throw new Error(`不是能用的图片：${name || '未命名'}（可用 PNG、JPG、GIF、WebP、BMP）`);
        const id = randomUUID();
        await writeFile(path.join(this.stickerDirectory, id + ext), picture);
        const title = path.basename(name, path.extname(name)).trim().slice(0, 60);
        list.unshift({ id, ext, name: title || '表情包', added: Date.now() });
      }
      await this.writeStickers(list);
      return list;
    };
    // Files named by path (a drag that carries file:// links, a copied file).
    const readPictures = async (paths: string[]) => Promise.all(paths.map(async file => {
      const info = await stat(file).catch(() => null);
      if (!info?.isFile()) throw new Error(`找不到图片：${path.basename(file)}`);
      if (info.size > STICKER_MAX_BYTES) throw new Error(`图片太大：${path.basename(file)}（最大 20 MB）`);
      return { name: path.basename(file), data: new Uint8Array(await readFile(file)) };
    }));
    handle('sticker-add', async (files) => {
      if (!Array.isArray(files)) throw new Error('参数无效。');
      return addStickers(files.map(file => {
        const entry = file as { name?: unknown; data?: unknown };
        if (!(entry.data instanceof Uint8Array)) throw new Error('图片无效。');
        return { name: typeof entry.name === 'string' ? entry.name.slice(0, 120) : '', data: entry.data };
      }));
    });
    handle('sticker-add-links', async (links) => {
      if (!Array.isArray(links) || !links.length || links.length > 50) throw new Error('一次最多添加 50 张。');
      const paths = links.map(link => {
        if (typeof link !== 'string' || !link.startsWith('file:') || link.length > 2048) throw new Error('只能添加这台电脑上的图片。');
        return fileURLToPath(link);
      });
      return addStickers(await readPictures(paths));
    });
    handle('sticker-add-url', async (link) => {
      if (typeof link !== 'string' || !link || link.length > 4096) throw new Error('图片地址无效。');
      return addStickers([await downloadPicture(link)]);
    });
    // Ctrl+V on the 表情包 page: a copied picture file (keeps a GIF moving),
    // else the copied picture itself, which the bridge saves to a file.
    handle('sticker-clipboard', async () => {
      const saved = path.join(os.tmpdir(), `localpinyin-sticker-${randomUUID()}`);
      try {
        const { file } = await this.options.bridge<{ file: string }>('clipboard-picture', [saved]);
        const [picture] = await readPictures([file]);
        return await addStickers([file === saved ? { ...picture, name: '粘贴的图片' } : picture]);
      } finally { await rm(saved, { force: true }).catch(() => undefined); }
    });
    handle('sticker-delete', async (id) => {
      const list = await this.stickers();
      const sticker = list.find(item => item.id === id);
      if (!sticker) return list;
      await rm(path.join(this.stickerDirectory, sticker.id + sticker.ext), { force: true });
      const rest = list.filter(item => item.id !== id);
      await this.writeStickers(rest);
      return rest;
    });
    handle('keep-open', (on) => { this.keepOpen = on === true; });
    handle('settings', () => { this.hide(); this.options.openSettings(); });
    ipcMain.on('localpinyin:panel:bootstrap', event => {
      event.returnValue = this.window && event.sender === this.window.webContents ? { dark: nativeTheme.shouldUseDarkColors } : null;
    });
  }
}
