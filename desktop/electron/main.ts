import { app, BrowserWindow, dialog, ipcMain, nativeTheme, protocol, session, shell } from 'electron';
import { execFile } from 'node:child_process';
import { mkdir, readFile } from 'node:fs/promises';
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { candidateStyleIds, clipboardRetentionChoices, type AppState, type Settings } from '../src/types';
import { exportBackup, inspectBackup, importBackup } from './backup';
import { clipboardFile, installClipboardHandlers } from './clipboard';
import { linkArguments, Panel, panelArguments, type PanelTarget } from './panel';
import { bridgeWithInput, VaultSession } from './vault';
import { checkForUpdate, downloadUpdate, initUpdates, installUpdate, releasePage, scheduleAutoCheck, setAutoCheck,
  updateStatus } from './updates';

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
// 记录: kept unlocked for a few minutes between uses (electron/vault.ts).
const vault = new VaultSession(bridgeWithInput(bridgePath));
const iconPath = app.isPackaged ? path.join(path.dirname(process.execPath), 'icon.ico')
  : path.resolve(app.getAppPath(), '../assets/branding/icon/LocalPinyinIME.ico');
function bridge<T>(command: string, args: string[] = []): Promise<T> {
  return new Promise((resolve, reject) => {
    execFile(bridgePath, [command, ...args], { windowsHide: true, encoding: 'utf8', timeout: 20_000,
      maxBuffer: 16 * 1024 * 1024 }, (error, stdout) => {
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
function fromSettings(event: Electron.IpcMainInvokeEvent | Electron.IpcMainEvent) {
  return !!window && event.sender === window.webContents && event.senderFrame === window.webContents.mainFrame &&
    event.senderFrame?.url === 'app://local/index.html';
}
function handle(channel: string, callback: (...args: unknown[]) => unknown) {
  ipcMain.handle('localpinyin:' + channel, (event, ...args) => {
    if (!fromSettings(event)) throw new Error('请求来源无效。');
    return callback(...args);
  });
}
// The settings the panel and the background behavior follow, kept from the
// last read so opening the panel does not wait for one.
let latestSettings: Settings | null = null;
async function state(): Promise<AppState> {
  const result = await bridge<AppState>('state');
  latestSettings = result.settings;
  nativeTheme.themeSource = result.settings.theme;
  window?.setBackgroundColor(nativeTheme.shouldUseDarkColors ? '#0f131b' : '#f4f3ef');
  panel?.window?.setBackgroundColor(nativeTheme.shouldUseDarkColors ? '#16181c' : '#f6f7f9');
  return result;
}
// One read at a time: the window's first request takes the read started at
// launch instead of starting another.
let pendingState: Promise<AppState> | null = null;
function sharedState(): Promise<AppState> {
  if (!pendingState) {
    pendingState = state();
    const clear = () => { pendingState = null; };
    pendingState.then(clear, clear);
  }
  return pendingState;
}
// The theme, read straight from settings.json so the window and its loading
// screen open in the right colors before the full settings arrive.
function savedTheme(): 'system' | 'light' | 'dark' {
  try {
    const text = readFileSync(path.join(dataRoot, 'LocalPinyinIME', 'settings.json'), 'utf8');
    const match = /"theme"\s*:\s*"(system|light|dark)"/.exec(text);
    return (match?.[1] as 'system' | 'light' | 'dark') ?? 'system';
  } catch { return 'system'; }
}
async function currentSettings(): Promise<Settings> {
  if (latestSettings) { void state().catch(() => undefined); return latestSettings; }
  return (await state()).settings;
}
const dataDirectory = () => bootstrap?.dataDirectory ?? path.join(dataRoot, 'LocalPinyinIME');
function installHandlers() {
  ipcMain.on('localpinyin:bootstrap', event => { event.returnValue = fromSettings(event) ? bootstrap : null; });
  handle('state', sharedState);
  const fuzzyIds = ['z-zh', 'c-ch', 's-sh', 'n-l', 'f-h', 'r-l', 'an-ang', 'en-eng', 'in-ing', 'ian-iang', 'uan-uang'];
  const domainIds = ['coding', 'work', 'math', 'study', 'daily', 'games'];
  const imeSwitchKeys = ['alt+capslock', 'ctrl+capslock', 'ctrl+alt+space', 'off'];
  handle('save-settings', (value) => {
    const settings = record(value);
    const fuzzy = settings.fuzzyPinyin;
    const domains = settings.usageDomains;
    if (!Array.isArray(fuzzy) || fuzzy.some(id => typeof id !== 'string' || !fuzzyIds.includes(id)) ||
        !Array.isArray(domains) || domains.some(id => typeof id !== 'string' || !domainIds.includes(id)) ||
        typeof settings.typoCorrection !== 'boolean' || typeof settings.mixedEnglish !== 'boolean' ||
        typeof settings.digitPeriod !== 'boolean' || typeof settings.digitPunctuation !== 'boolean' ||
        typeof settings.letterPunctuation !== 'boolean' || typeof settings.codePunctuation !== 'boolean' ||
        typeof settings.tabTranslate !== 'boolean' || typeof settings.panelDoubleTab !== 'boolean' ||
        typeof settings.panelResident !== 'boolean' || typeof settings.emojiCandidates !== 'boolean' ||
        typeof settings.englishAutocorrect !== 'boolean' || typeof settings.englishSentence !== 'boolean' ||
        typeof settings.englishIme !== 'boolean' ||
        !imeSwitchKeys.includes(String(settings.imeSwitchKey)) ||
        !['ask', 'on', 'off'].includes(String(settings.clipboardHistory)) ||
        !(clipboardRetentionChoices as readonly number[]).includes(settings.clipboardRetentionDays as number) ||
        !['none', 'xiaohe', 'ziranma'].includes(String(settings.doublePinyin)))
      throw new Error('设置选项无效。');
    if (!['system', 'light', 'dark'].includes(String(settings.theme)) ||
        !(candidateStyleIds as readonly string[]).includes(String(settings.candidateStyle)) || !Number.isInteger(settings.textSize) ||
        (settings.textSize as number) < 1 || (settings.textSize as number) > 7 || typeof settings.shiftToggleEnabled !== 'boolean' ||
        typeof settings.predictionEnabled !== 'boolean')
      throw new Error('设置选项无效。');
    return enqueue(async () => {
      const before = latestSettings?.clipboardHistory;
      // Fuzzy pairs in the canonical order the native settings use.
      const pairs = fuzzyIds.filter(id => (fuzzy as string[]).includes(id)).join(',');
      const scenes = domainIds.filter(id => (domains as string[]).includes(id)).join(',');
      await bridge('save-settings', [String(settings.theme), String(settings.textSize), String(settings.shiftToggleEnabled),
        String(settings.predictionEnabled), String(settings.candidateStyle), `typoCorrection=${settings.typoCorrection}`,
        `mixedEnglish=${settings.mixedEnglish}`, `fuzzyPinyin=${pairs}`, `doublePinyin=${settings.doublePinyin}`,
        `usageDomains=${scenes}`, `digitPeriod=${settings.digitPeriod}`, `digitPunctuation=${settings.digitPunctuation}`,
        `letterPunctuation=${settings.letterPunctuation}`, `codePunctuation=${settings.codePunctuation}`,
        `tabTranslate=${settings.tabTranslate}`, `panelDoubleTab=${settings.panelDoubleTab}`,
        `panelResident=${settings.panelResident}`, `emojiCandidates=${settings.emojiCandidates}`,
        `englishAutocorrect=${settings.englishAutocorrect}`, `englishSentence=${settings.englishSentence}`,
        `englishIme=${settings.englishIme}`, `imeSwitchKey=${settings.imeSwitchKey}`,
        `clipboardHistory=${settings.clipboardHistory}`, `clipboardRetentionDays=${settings.clipboardRetentionDays}`]);
      // The recorder starts with the setting and leaves with it, also at sign-in.
      if (settings.clipboardHistory !== before)
        await bridge('clipboard-agent', [settings.clipboardHistory === 'on' ? 'start' : 'stop']).catch(() => undefined);
      return state();
    });
  });
  handle('domain-words', () => bridge('domain-words'));
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
  handle('learning-list', (query) => bridge('learning-list', [text(query, 64, true)]));
  handle('learning-forget', (pinyin, word) => enqueue(() => bridge('learning-forget', [text(pinyin, 256), text(word, 128)])));
  handle('phrases', () => bridge('phrases-list'));
  handle('phrase-add', (content, label) => enqueue(() => bridge('phrases-add', [text(content, 64), text(label, 32)])));
  handle('phrase-delete', (pinyin, content) => enqueue(() => bridge('phrases-delete', [text(pinyin, 64), text(content, 64)])));
  handle('emoji-candidates', () => bridge('emoji-list'));
  handle('emoji-candidate-delete', (pinyin, content) => enqueue(() => bridge('emoji-delete', [text(pinyin, 64), text(content, 32)])));
  handle('clipboard-clear', () => enqueue(() => bridge('clipboard-clear')));
  handle('clipboard-summary', () => bridge('clipboard-summary'));
  handle('vault-status', () => vault.status());
  handle('vault-create', (password) => enqueue(async () => { await vault.create(password); return vault.status(); }));
  handle('vault-password', (current, next) => enqueue(() => vault.changePassword(current, next)));
  handle('vault-destroy', () => enqueue(() => vault.destroy()));
  handle('backup-export', (parts, file) => exportBackup(backupContext(), parts, chosenFile(file)));
  handle('backup-inspect', (file) => inspectBackup(chosenFile(file)));
  handle('backup-import', (file, choices) => enqueue(async () => {
    const result = await importBackup(backupContext(), chosenFile(file), choices);
    await state();
    return result;
  }));
  handle('backup-choose', (mode, name) => chooseBackupFile(mode, name));
  handle('open-data', async () => {
    const current = await bridge<AppState>('state');
    await mkdir(current.dataDirectory, { recursive: true });
    const error = await shell.openPath(current.dataDirectory);
    if (error) throw new Error('无法打开数据目录。');
  });
  handle('update-status', () => updateStatus());
  handle('update-check', () => checkForUpdate());
  handle('update-download', () => downloadUpdate());
  handle('update-install', () => installUpdate(() => app.quit()));
  handle('update-auto', (enabled) => setAutoCheck(enabled));
  handle('update-page', async () => {
    if (process.env.LOCALPINYINIME_UPDATE_FEED) return;
    await shell.openExternal(releasePage());
  });
  handle('window', (action) => {
    if (action === 'minimize') window?.minimize();
    else if (action === 'maximize') window?.isMaximized() ? window.unmaximize() : window?.maximize();
    else if (action === 'close') window?.close();
    else throw new Error('窗口操作无效。');
  });
}
function backupContext() {
  return { dataDirectory: dataDirectory(), userData: app.getPath('userData'), bridge, version: app.getVersion(),
    reloadPanel: () => panel?.reloadState() };
}
// Backups are read and written only where the user pointed a file dialog.
const chosenFiles = new Set<string>();
function chosenFile(value: unknown): string {
  if (typeof value !== 'string' || !chosenFiles.has(value)) throw new Error('请先选择备份文件。');
  return value;
}
async function chooseBackupFile(mode: unknown, name: unknown): Promise<string | null> {
  const chosen = await showBackupDialog(mode, name);
  if (chosen) chosenFiles.add(chosen);
  return chosen;
}
async function showBackupDialog(mode: unknown, name: unknown): Promise<string | null> {
  if (!window) return null;
  // The test suite has no one to click the file dialog.
  if (hidden && process.env.LOCALPINYINIME_QA_BACKUP_FILE) return process.env.LOCALPINYINIME_QA_BACKUP_FILE;
  const filters = [{ name: '本地拼音备份', extensions: ['lpbackup'] }];
  if (mode === 'save') {
    const result = await dialog.showSaveDialog(window, { title: '导出用户数据', filters,
      defaultPath: path.join(app.getPath('documents'), typeof name === 'string' && /^[\w一-鿿.-]{1,80}$/.test(name) ? name : '本地拼音备份.lpbackup') });
    return result.canceled ? null : result.filePath ?? null;
  }
  const result = await dialog.showOpenDialog(window, { title: '导入用户数据', filters, properties: ['openFile'] });
  return result.canceled ? null : result.filePaths[0] ?? null;
}
const csp = "default-src 'none'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data: blob:; font-src 'self'; connect-src 'none'; base-uri 'none'; form-action 'none'; frame-src 'none'; object-src 'none'";
let panel: Panel | null = null;
function nativeHandle(target: BrowserWindow | null | undefined) {
  if (!target || target.isDestroyed()) return '';
  const handle = target.getNativeWindowHandle();
  return (handle.length >= 8 ? handle.readBigUInt64LE(0) : BigInt(handle.readUInt32LE(0))).toString();
}
function openPanel(target: PanelTarget) {
  // Tab twice inside the panel or the settings window itself is ignored.
  if ([nativeHandle(window), nativeHandle(panel?.window)].includes(target.host)) return;
  void panel?.open(target).catch(error => console.error('Panel failed:', error.message));
}
async function openSettings() {
  if (window) {
    if (!hidden) { if (window.isMinimized()) window.restore(); window.show(); window.focus(); }
    return;
  }
  // Shown at once: the page draws its loading screen first and asks for the
  // settings itself, so nothing here waits for the native bridge.
  const created = new BrowserWindow({ width: 1160, height: 800, minWidth: 920, minHeight: 680,
    frame: false, show: !hidden, backgroundColor: nativeTheme.shouldUseDarkColors ? '#0f131b' : '#f4f3ef', title: '本地拼音设置',
    icon: iconPath, webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'), contextIsolation: true, sandbox: true,
      nodeIntegration: false, webSecurity: true, backgroundThrottling: !hidden, offscreen: hidden, spellcheck: false,
      devTools: !app.isPackaged } });
  window = created;
  created.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
  created.webContents.on('will-navigate', event => event.preventDefault());
  created.webContents.on('will-attach-webview', event => event.preventDefault());
  created.on('closed', () => {
    if (window === created) window = null;
    void panel?.settingsClosed();
  });
  await created.loadURL('app://local/index.html');
}
const mediaTypes: Record<string, string> = { '.png': 'image/png', '.gif': 'image/gif', '.jpg': 'image/jpeg', '.jpeg': 'image/jpeg',
  '.webp': 'image/webp', '.bmp': 'image/bmp' };
// Pictures the panel shows: the user's stickers and copied images.
async function media(pathname: string): Promise<{ file: string; type: string } | null> {
  const [, kind, name] = /^\/media\/(stickers|clipboard)\/([^/]+)$/.exec(pathname) ?? [];
  if (kind === 'stickers') return panel ? panel.stickerFile(name) : null;
  if (kind === 'clipboard') { const file = clipboardFile(dataDirectory(), name); return file ? { file, type: 'image/png' } : null; }
  return null;
}
if (!app.requestSingleInstanceLock()) app.quit();
else {
  const initialPanel = panelArguments(linkArguments(process.argv));
  app.on('second-instance', (_event, argv) => {
    const target = panelArguments(linkArguments(argv));
    if (target) openPanel(target);
    else if (argv.includes('--first-run')) { if (latestSettings?.clipboardHistory === 'ask') void openSettings(); }
    else void openSettings();
  });
  // Read the settings while Electron starts up; the window opens without
  // waiting for them and its first request takes this read.
  const initialState = bridge<AppState>('state');
  initialState.catch(() => undefined);
  pendingState = initialState.then(result => {
    bootstrap = result;
    latestSettings = result.settings;
    if (app.isReady()) nativeTheme.themeSource = result.settings.theme;
    return result;
  });
  pendingState.then(() => { pendingState = null; }, () => { pendingState = null; });
  app.whenReady().then(async () => {
    nativeTheme.themeSource = savedTheme();
    const dist = path.join(app.getAppPath(), 'dist');
    protocol.handle('app', async request => {
      try {
        const url = new URL(request.url);
        if (url.host !== 'local' || !['GET', 'HEAD'].includes(request.method)) return new Response('', { status: 403 });
        const pathname = decodeURIComponent(url.pathname);
        if (pathname.startsWith('/media/')) {
          const found = await media(pathname);
          if (!found) return new Response('', { status: 404 });
          const body = await readFile(found.file);
          return new Response(request.method === 'HEAD' ? null : body, { headers: { 'Content-Type': found.type,
            'X-Content-Type-Options': 'nosniff', 'Cache-Control': 'no-store' } });
        }
        const filename = path.resolve(dist, '.' + pathname);
        const relative = path.relative(dist, filename);
        if (!relative || relative.startsWith('..') || path.isAbsolute(relative)) return new Response('', { status: 403 });
        const mime: Record<string, string> = { '.html': 'text/html; charset=utf-8', '.js': 'application/javascript',
          '.css': 'text/css', '.svg': 'image/svg+xml', '.png': 'image/png', '.woff2': 'font/woff2', '.json': 'application/json',
          ...mediaTypes };
        const body = await readFile(filename);
        return new Response(request.method === 'HEAD' ? null : body, { headers: {
          'Content-Type': mime[path.extname(filename)] || 'application/octet-stream',
          'Content-Security-Policy': csp, 'X-Content-Type-Options': 'nosniff', 'Cache-Control': 'no-store' } });
      } catch { return new Response('', { status: 404 }); }
    });
    session.defaultSession.setPermissionRequestHandler((_contents, _permission, callback) => callback(false));
    session.defaultSession.setPermissionCheckHandler(() => false);
    session.defaultSession.webRequest.onBeforeRequest((details, callback) => {
      callback({ cancel: !details.url.startsWith('app://local/') && !details.url.startsWith('devtools://') &&
        !details.url.startsWith('blob:app://local/') });
    });
    installHandlers();
    const settingsFirst = !initialPanel && !process.argv.includes('--first-run');
    if (settingsFirst) void openSettings();
    bootstrap = await initialState;
    latestSettings = bootstrap.settings;
    nativeTheme.themeSource = bootstrap.settings.theme;
    panel = new Panel({ dataDirectory: dataDirectory(), userData: app.getPath('userData'), bridge,
      settings: currentSettings, preload: path.join(__dirname, 'panel-preload.cjs'), icon: iconPath, hidden,
      settingsOpen: () => !!window, openSettings: () => void openSettings(), quit: () => app.quit() });
    panel.resident = bootstrap.settings.panelResident;
    panel.install();
    panel.handle('vault-status', () => vault.status());
    panel.handle('vault-create', (password) => enqueue(() => vault.create(password)));
    panel.handle('vault-unlock', (password) => vault.unlock(password));
    panel.handle('vault-items', () => vault.items());
    panel.handle('vault-save', (item) => enqueue(() => vault.save(item)));
    panel.handle('vault-delete', (id) => enqueue(() => vault.remove(id)));
    panel.handle('vault-lock', () => vault.lock());
    panel.handle('vault-copy', (value) => vault.copy(value));
    installClipboardHandlers({ panel, dataDirectory, bridge, settings: currentSettings,
      saveSettings: async (patch) => {
        const current = await currentSettings();
        const next = { ...current, ...patch };
        await bridge('save-settings', [next.theme, String(next.textSize), String(next.shiftToggleEnabled), String(next.predictionEnabled),
          next.candidateStyle, `clipboardHistory=${next.clipboardHistory}`, `clipboardRetentionDays=${next.clipboardRetentionDays}`]);
        if (patch.clipboardHistory) await bridge('clipboard-agent', [patch.clipboardHistory === 'on' ? 'start' : 'stop']).catch(() => undefined);
        window?.webContents.send('localpinyin:settings-changed');
        return (await state()).settings;
      } });
    await initUpdates(status => window?.webContents.send('localpinyin:update-status', status));
    if (initialPanel) {
      openPanel(initialPanel);
    } else if (process.argv.includes('--first-run') && bootstrap.settings.clipboardHistory !== 'ask') {
      // Started by the installer to ask about the clipboard: already answered.
      if (!panel.keepsApp()) app.quit();
    } else {
      // Measurements of the packaged app (QA headless without a test feed) stay offline.
      if (!hidden || process.env.LOCALPINYINIME_UPDATE_FEED) scheduleAutoCheck();
    }
    if (bootstrap.settings.clipboardHistory === 'on') void bridge('clipboard-agent', ['start']).catch(() => undefined);
  }).catch(error => { console.error('Settings startup failed:', error.message); app.quit(); });
}
// Closing windows is handled above: the settings window may leave the panel
// waiting in the background.
app.on('window-all-closed', () => { if (!panel?.keepsApp()) app.quit(); });
