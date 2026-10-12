import { app, BrowserWindow, ipcMain, nativeTheme, protocol, session, shell } from 'electron';
import { execFile } from 'node:child_process';
import { mkdir, readFile } from 'node:fs/promises';
import path from 'node:path';
import type { AppState } from '../src/types';

protocol.registerSchemesAsPrivileged([{ scheme: 'app', privileges: { standard: true, secure: true, supportFetchAPI: true } }]);
app.setName('本地拼音设置');
const dataRoot = process.env.LOCALPINYINIME_LOCALAPPDATA || process.env.LOCALAPPDATA || app.getPath('appData');
app.setPath('userData', path.join(dataRoot, 'LocalPinyinIME', 'desktop'));
app.commandLine.appendSwitch('lang', 'zh-CN');
app.commandLine.appendSwitch('disable-component-update');
app.commandLine.appendSwitch('disable-background-networking');
app.commandLine.appendSwitch('disable-features', 'AutofillServerCommunication,MediaRouter,OptimizationHints');
const hidden = process.env.LOCALPINYINIME_QA_HEADLESS === '1';
let window: BrowserWindow | null = null;
let bootstrap: AppState | null = null;
let mutations: Promise<unknown> = Promise.resolve();
const bridgePath = app.isPackaged ? path.join(path.dirname(process.execPath), 'LocalPinyinSettingsBridge.exe')
  : process.env.LOCALPINYINIME_BRIDGE_PATH || path.resolve(app.getAppPath(), '../.build/bin/LocalPinyinSettingsBridge.exe');
function bridge<T>(command: string, args: string[] = []): Promise<T> {
  return new Promise((resolve, reject) => {
    execFile(bridgePath, [command, ...args], { windowsHide: true, encoding: 'utf8', timeout: 20_000,
      maxBuffer: 4 * 1024 * 1024 }, (error, stdout) => {
      let result: T & { error?: string };
      try { result = JSON.parse(stdout); } catch { reject(new Error('本地服务暂时不可用，请使用完整安装包启动设置。')); return; }
      if (error || result.error) reject(new Error(result.error || '本地操作失败，请重试。'));
      else resolve(result);
    });
  });
}
function enqueue<T>(operation: () => Promise<T>): Promise<T> {
  const result = mutations.then(operation);
  mutations = result.then(() => undefined, () => undefined);
  return result;
}
function text(value: unknown, maximum: number, empty = false): string {
  if (typeof value !== 'string' || value.length > maximum || (!empty && !value.trim()) || /[\0\r\n\t]/.test(value))
    throw new Error('输入内容无效。');
  return value;
}
function record(value: unknown): Record<string, unknown> {
  if (!value || typeof value !== 'object' || Array.isArray(value)) throw new Error('参数无效。');
  return value as Record<string, unknown>;
}
function entryArgs(value: unknown): string[] {
  const entry = record(value);
  if (!Number.isInteger(entry.frequency) || (entry.frequency as number) < 0 || (entry.frequency as number) > 1_000_000)
    throw new Error('词频应为0至1000000之间的整数。');
  return [text(entry.pinyin, 256), text(entry.word, 128), String(entry.frequency)];
}
function handle(channel: string, callback: (...args: unknown[]) => unknown) {
  ipcMain.handle('localpinyin:' + channel, (event, ...args) => {
    if (!window || event.sender !== window.webContents || event.senderFrame !== window.webContents.mainFrame ||
        event.senderFrame?.url !== 'app://local/index.html') throw new Error('请求来源无效。');
    return callback(...args);
  });
}
async function state(): Promise<AppState> {
  const result = await bridge<AppState>('state');
  nativeTheme.themeSource = result.settings.theme;
  window?.setBackgroundColor(nativeTheme.shouldUseDarkColors ? '#13151d' : '#f5f6fa');
  return result;
}
function installHandlers() {
  ipcMain.on('localpinyin:bootstrap', event => {
    event.returnValue = window && event.sender === window.webContents &&
      event.senderFrame === window.webContents.mainFrame && event.senderFrame?.url === 'app://local/index.html' ? bootstrap : null;
  });
  handle('state', state);
  handle('save-settings', (value) => {
    const settings = record(value);
    if (!['system', 'light', 'dark'].includes(String(settings.theme)) || !Number.isInteger(settings.textSize) ||
        (settings.textSize as number) < 1 || (settings.textSize as number) > 7 || typeof settings.shiftToggleEnabled !== 'boolean')
      throw new Error('设置选项无效。');
    return enqueue(async () => {
      await bridge('save-settings', [String(settings.theme), String(settings.textSize), String(settings.shiftToggleEnabled)]);
      return state();
    });
  });
  handle('lookup', (pinyin) => bridge('lookup', [text(pinyin, 256, true)]));
  handle('user-list', () => bridge('user-list'));
  handle('user-upsert', (entry, previous) => {
    const args = entryArgs(entry);
    if (previous !== undefined) {
      const old = record(previous);
      args.push(text(old.pinyin, 256), text(old.word, 128));
    }
    return enqueue(() => bridge('user-upsert', args));
  });
  handle('user-delete', (entry) => {
    const old = record(entry);
    return enqueue(() => bridge('user-delete', [text(old.pinyin, 256), text(old.word, 128)]));
  });
  handle('dictionary-search', (query, limit = 100) => {
    if (!Number.isInteger(limit) || (limit as number) < 1 || (limit as number) > 500) throw new Error('查询数量无效。');
    return bridge('dictionary-search', [text(query, 256, true), String(limit)]);
  });
  handle('clear-learning', () => enqueue(() => bridge('clear-learning')));
  handle('open-data', async () => {
    const current = await bridge<AppState>('state');
    await mkdir(current.dataDirectory, { recursive: true });
    const error = await shell.openPath(current.dataDirectory);
    if (error) throw new Error('无法打开数据目录。');
  });
  handle('window', (action) => {
    if (action === 'minimize') window?.minimize();
    else if (action === 'maximize') window?.isMaximized() ? window.unmaximize() : window?.maximize();
    else if (action === 'close') window?.close();
    else throw new Error('窗口操作无效。');
  });
}
const csp = "default-src 'none'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; font-src 'self'; connect-src 'none'; base-uri 'none'; form-action 'none'; frame-src 'none'; object-src 'none'";
if (!app.requestSingleInstanceLock()) app.quit();
else {
  app.on('second-instance', () => { if (!hidden && window) { if (window.isMinimized()) window.restore(); window.show(); window.focus(); } });
  app.whenReady().then(async () => {
    bootstrap = await bridge<AppState>('state');
    nativeTheme.themeSource = bootstrap.settings.theme;
    const dist = path.join(app.getAppPath(), 'dist');
    protocol.handle('app', async request => {
      try {
        const url = new URL(request.url);
        const filename = path.resolve(dist, '.' + decodeURIComponent(url.pathname));
        const relative = path.relative(dist, filename);
        if (url.host !== 'local' || !relative || relative.startsWith('..') || path.isAbsolute(relative) ||
            !['GET', 'HEAD'].includes(request.method)) return new Response('', { status: 403 });
        const mime: Record<string, string> = { '.html': 'text/html; charset=utf-8', '.js': 'application/javascript',
          '.css': 'text/css', '.svg': 'image/svg+xml', '.png': 'image/png', '.woff2': 'font/woff2' };
        const body = await readFile(filename);
        return new Response(request.method === 'HEAD' ? null : body, { headers: {
          'Content-Type': mime[path.extname(filename)] || 'application/octet-stream',
          'Content-Security-Policy': csp, 'X-Content-Type-Options': 'nosniff', 'Cache-Control': 'no-store' } });
      } catch { return new Response('', { status: 404 }); }
    });
    session.defaultSession.setPermissionRequestHandler((_contents, _permission, callback) => callback(false));
    session.defaultSession.setPermissionCheckHandler(() => false);
    session.defaultSession.webRequest.onBeforeRequest((details, callback) => {
      callback({ cancel: !details.url.startsWith('app://local/') && !details.url.startsWith('devtools://') });
    });
    window = new BrowserWindow({ width: 1160, height: 800, minWidth: 920, minHeight: 680,
      frame: false, show: false, backgroundColor: nativeTheme.shouldUseDarkColors ? '#13151d' : '#f5f6fa', title: '本地拼音设置',
      icon: app.isPackaged ? path.join(path.dirname(process.execPath), 'icon.ico')
        : path.resolve(app.getAppPath(), '../assets/branding/icon/LocalPinyinIME.ico'), webPreferences: {
        preload: path.join(__dirname, 'preload.cjs'), contextIsolation: true, sandbox: true,
        nodeIntegration: false, webSecurity: true, backgroundThrottling: !hidden, offscreen: hidden, spellcheck: false,
        devTools: !app.isPackaged } });
    window.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
    window.webContents.on('will-navigate', event => event.preventDefault());
    window.webContents.on('will-attach-webview', event => event.preventDefault());
    window.on('closed', () => { window = null; });
    installHandlers();
    window.once('ready-to-show', () => { if (!hidden) window?.show(); });
    await window.loadURL('app://local/index.html');
  }).catch(error => { console.error('Settings startup failed:', error.message); app.quit(); });
}
app.on('window-all-closed', () => app.quit());
