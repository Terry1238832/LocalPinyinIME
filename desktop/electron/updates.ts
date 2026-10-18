// Updates: the settings app asks the project's GitHub releases whether a newer
// version exists, downloads its installer, checks the installer against the
// release's SHA-256 file and starts it. The input method itself never goes
// online; this reads release information and downloads an installer, and
// sends nothing about the user. Requests go through a session of their own
// that only reaches the project's release endpoints.
import { app, session, type Session } from 'electron';
import { spawn } from 'node:child_process';
import { createHash } from 'node:crypto';
import { createWriteStream } from 'node:fs';
import { mkdir, readFile, rename, rm, writeFile } from 'node:fs/promises';
import path from 'node:path';
import type { UpdateStatus } from '../src/types';

const REPOSITORY = 'Terry1238832/LocalPinyinIME';
// The test suite serves releases from 127.0.0.1 and never starts an installer.
const testFeed = process.env.LOCALPINYINIME_UPDATE_FEED;
const FEED = testFeed || `https://api.github.com/repos/${REPOSITORY}/releases?per_page=30`;
const dryRun = process.env.LOCALPINYINIME_UPDATE_DRY_RUN === '1';
const ALLOWED = testFeed ? [new URL(testFeed).origin + '/'] : [
  `https://api.github.com/repos/${REPOSITORY}/`,
  `https://github.com/${REPOSITORY}/releases/`,
  'https://objects.githubusercontent.com/',
  'https://release-assets.githubusercontent.com/',
];
const RELEASE_PAGES = testFeed ? new URL(testFeed).origin + '/' : `https://github.com/${REPOSITORY}/releases/`;
// Opening settings checks at most this often.
const AUTO_INTERVAL_MS = 12 * 60 * 60 * 1000;
const MAX_INSTALLER_BYTES = 400 * 1024 * 1024;

type Release = { version: string; page: string; notes: string; published: string; installer?: { name: string; size: number; url: string }; checksum?: string };
// What the last check found survives a restart: "current" is shown as is,
// "available" is checked again at once for fresh download links.
type Preferences = { autoCheck: boolean; checkedAt?: string; lastResult?: 'current' | 'available' };

let network: Session | null = null;
let release: Release | null = null;
let installerPath: string | null = null;
let preferences: Preferences = { autoCheck: true };
let status: UpdateStatus = { state: 'idle', current: app.getVersion(), autoCheck: true };
let publish: (value: UpdateStatus) => void = () => undefined;

function preferencesFile() { return path.join(app.getPath('userData'), 'update.json'); }
function downloadDirectory() { return path.join(app.getPath('userData'), '..', 'updates'); }

function updateSession(): Session {
  if (network) return network;
  network = session.fromPartition('localpinyin-updates', { cache: false });
  network.webRequest.onBeforeRequest((details, callback) => {
    callback({ cancel: !ALLOWED.some(prefix => details.url.startsWith(prefix)) });
  });
  network.setPermissionRequestHandler((_contents, _permission, callback) => callback(false));
  return network;
}

function set(patch: Partial<UpdateStatus>) {
  status = { ...status, ...patch, current: app.getVersion(), autoCheck: preferences.autoCheck, checkedAt: preferences.checkedAt };
  publish(status);
}

function numbers(version: string): number[] | null {
  const match = /^v?(\d+)\.(\d+)\.(\d+)$/.exec(version.trim());
  return match ? match.slice(1).map(Number) : null;
}

// Positive when `a` is newer than `b`.
export function compareVersions(a: string, b: string): number {
  const left = numbers(a) ?? [0, 0, 0], right = numbers(b) ?? [0, 0, 0];
  for (let index = 0; index < 3; ++index) if (left[index] !== right[index]) return left[index] - right[index];
  return 0;
}

async function savePreferences() {
  await mkdir(path.dirname(preferencesFile()), { recursive: true });
  await writeFile(preferencesFile(), JSON.stringify(preferences, null, 2) + '\n', 'utf8');
}

export async function initUpdates(listener: (value: UpdateStatus) => void): Promise<UpdateStatus> {
  publish = listener;
  try {
    const saved = JSON.parse(await readFile(preferencesFile(), 'utf8')) as Partial<Preferences>;
    preferences = { autoCheck: saved.autoCheck !== false,
      checkedAt: typeof saved.checkedAt === 'string' && !Number.isNaN(Date.parse(saved.checkedAt)) ? saved.checkedAt : undefined,
      lastResult: saved.lastResult === 'current' || saved.lastResult === 'available' ? saved.lastResult : undefined };
  } catch { preferences = { autoCheck: true }; }
  set({ state: preferences.checkedAt && preferences.lastResult === 'current' ? 'current' : 'idle' });
  return status;
}

export function updateStatus(): UpdateStatus { return status; }

export async function setAutoCheck(value: unknown): Promise<UpdateStatus> {
  if (typeof value !== 'boolean') throw new Error('设置选项无效。');
  preferences = { ...preferences, autoCheck: value };
  await savePreferences();
  set({});
  return status;
}

async function fetchChecked(url: string, accept: string): Promise<Response> {
  if (!ALLOWED.some(prefix => url.startsWith(prefix))) throw new Error('更新地址无效。');
  const response = await updateSession().fetch(url, { headers: { Accept: accept, 'User-Agent': `LocalPinyinIME/${app.getVersion()}` },
    signal: AbortSignal.timeout(20_000) });
  if (!response.ok) throw new Error(`更新服务器返回 ${response.status}。`);
  return response;
}

export async function checkForUpdate(): Promise<UpdateStatus> {
  if (status.state === 'checking' || status.state === 'downloading') return status;
  set({ state: 'checking', error: undefined });
  try {
    const list = await (await fetchChecked(FEED, 'application/vnd.github+json')).json() as unknown;
    if (!Array.isArray(list)) throw new Error('更新信息格式不正确。');
    let newest: Release | null = null;
    for (const item of list) {
      if (!item || typeof item !== 'object' || item.draft) continue;
      const tag = String(item.tag_name ?? '');
      if (!numbers(tag)) continue;
      const version = tag.replace(/^v/, '');
      if (newest && compareVersions(version, newest.version) <= 0) continue;
      const assets = Array.isArray(item.assets) ? item.assets : [];
      const asset = (name: string) => assets.find((entry: { name?: unknown }) => entry?.name === name);
      const installer = asset(`LocalPinyinIME-${version}-x64.exe`);
      const checksum = asset(`LocalPinyinIME-${version}-x64.exe.sha256`);
      newest = {
        version,
        page: typeof item.html_url === 'string' && item.html_url.startsWith(RELEASE_PAGES) ? item.html_url : RELEASE_PAGES,
        notes: typeof item.body === 'string' ? item.body.slice(0, 4000) : '',
        published: typeof item.published_at === 'string' ? item.published_at : '',
        installer: installer && typeof installer.browser_download_url === 'string' && Number.isInteger(installer.size)
          ? { name: installer.name, size: installer.size, url: installer.browser_download_url } : undefined,
        checksum: checksum && typeof checksum.browser_download_url === 'string' ? checksum.browser_download_url : undefined,
      };
    }
    const newer = Boolean(newest && compareVersions(newest.version, app.getVersion()) > 0);
    preferences = { ...preferences, checkedAt: new Date().toISOString(), lastResult: newer ? 'available' : 'current' };
    await savePreferences().catch(() => undefined);
    if (!newest || !newer) {
      release = null;
      set({ state: 'current', latest: undefined, progress: undefined });
    } else {
      if (release?.version !== newest.version) installerPath = null;
      release = newest;
      set({ state: installerPath ? 'ready' : 'available', progress: undefined, latest: {
        version: newest.version, notes: newest.notes, published: newest.published,
        size: newest.installer?.size ?? 0, downloadable: Boolean(newest.installer && newest.checksum) } });
    }
  } catch (error) {
    set({ state: 'error', error: error instanceof Error && !/fetch failed|net::/i.test(error.message)
      ? error.message : '无法连接到 GitHub，请检查网络后重试。' });
  }
  return status;
}

export async function downloadUpdate(): Promise<UpdateStatus> {
  const target = release;
  if (status.state !== 'available' || !target?.installer || !target.checksum) throw new Error('没有可以下载的更新。');
  const installer = target.installer;
  if (!/^LocalPinyinIME-\d+\.\d+\.\d+-x64\.exe$/.test(installer.name) || installer.size <= 0 || installer.size > MAX_INSTALLER_BYTES)
    throw new Error('安装包信息无效。');
  set({ state: 'downloading', progress: 0, error: undefined });
  const directory = downloadDirectory();
  const partial = path.join(directory, installer.name + '.partial');
  try {
    const expected = (await (await fetchChecked(target.checksum, 'application/octet-stream')).text()).trim().split(/\s+/)[0].toLowerCase();
    if (!/^[0-9a-f]{64}$/.test(expected)) throw new Error('校验文件格式不正确。');
    await mkdir(directory, { recursive: true });
    const response = await fetchChecked(installer.url, 'application/octet-stream');
    if (!response.body) throw new Error('下载没有内容。');
    const hash = createHash('sha256');
    const file = createWriteStream(partial);
    const reader = response.body.getReader();
    let received = 0, reported = 0;
    try {
      for (;;) {
        const { done, value } = await reader.read();
        if (done) break;
        received += value.byteLength;
        if (received > MAX_INSTALLER_BYTES) throw new Error('下载的文件过大。');
        hash.update(value);
        if (!file.write(value)) await new Promise(resolve => file.once('drain', resolve));
        if (received - reported > installer.size / 100) {
          reported = received;
          set({ progress: Math.min(0.99, received / installer.size) });
        }
      }
    } finally {
      await new Promise<void>((resolve, reject) => file.end((error?: Error | null) => error ? reject(error) : resolve()));
    }
    if (received !== installer.size) throw new Error('下载不完整，请重试。');
    if (hash.digest('hex') !== expected) throw new Error('安装包校验失败（SHA-256 不一致），已删除下载的文件。');
    const final = path.join(directory, installer.name);
    await rm(final, { force: true });
    await rename(partial, final);
    installerPath = final;
    set({ state: 'ready', progress: 1 });
  } catch (error) {
    await rm(partial, { force: true }).catch(() => undefined);
    installerPath = null;
    set({ state: 'error', progress: undefined, error: error instanceof Error ? error.message : '下载失败，请重试。' });
  }
  return status;
}

// Starts the verified installer (it asks for administrator rights itself) and
// closes the settings app so its files can be replaced.
export async function installUpdate(quit: () => void): Promise<{ started: boolean }> {
  if (status.state !== 'ready' || !installerPath) throw new Error('更新还没有下载完成。');
  if (dryRun) return { started: true };
  const child = spawn(installerPath, [], { detached: true, stdio: 'ignore', windowsHide: false });
  child.unref();
  setTimeout(quit, 400);
  return { started: true };
}

export function releasePage(): string {
  return release?.page && release.page.startsWith(RELEASE_PAGES) ? release.page : RELEASE_PAGES;
}

// Opening settings checks in the background, at most every twelve hours.
export function scheduleAutoCheck() {
  if (!preferences.autoCheck) return;
  const last = preferences.checkedAt ? Date.parse(preferences.checkedAt) : 0;
  if (Date.now() - last < AUTO_INTERVAL_MS && preferences.lastResult !== 'available') return;
  setTimeout(() => void checkForUpdate(), testFeed ? 300 : 4000);
}
