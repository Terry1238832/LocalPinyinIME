import { packager } from '@electron/packager';
import { cp, mkdir, readFile, readdir, rename, rm, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { createHash } from 'node:crypto';
const desktop = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const workspace = path.resolve(desktop, '..');
const stage = path.join(workspace, '.build/electron-app');
const output = path.join(workspace, '.build/electron-dist');
function checked(directory) {
  const relative = path.relative(path.join(workspace, '.build'), directory);
  if (!relative || relative.startsWith('..') || path.isAbsolute(relative)) throw new Error('Unsafe package path');
  return directory;
}
checked(stage); checked(output);
await rm(stage, { recursive: true, force: true });
await mkdir(stage, { recursive: true });
await cp(path.join(desktop, 'dist'), path.join(stage, 'dist'), { recursive: true });
await mkdir(path.join(stage, 'electron'), { recursive: true });
for (const name of ['main.cjs','preload.cjs']) await cp(path.join(desktop, 'electron', name), path.join(stage, 'electron', name));
await cp(path.join(workspace,'assets/branding/icon/LocalPinyinIME.ico'),path.join(stage,'icon.ico'));
const meta = JSON.parse(await readFile(path.join(desktop, 'package.json'), 'utf8'));
const checksums = JSON.parse(await readFile(path.join(desktop, 'node_modules/electron/checksums.json'), 'utf8'));
const cache = path.join(workspace, '.build/electron-cache');
const archiveName = `electron-v${meta.devDependencies.electron}-win32-x64.zip`;
async function findArchive(directory) {
  let files;
  try { files = await readdir(directory, { withFileTypes: true }); } catch (error) { if (error.code === 'ENOENT') return null; throw error; }
  for (const file of files) {
    const filename = path.join(directory, file.name);
    if (file.isFile() && file.name === archiveName) return filename;
    if (file.isDirectory()) { const nested = await findArchive(filename); if (nested) return nested; }
  }
  return null;
}
const archive = await findArchive(cache);
if (archive && createHash('sha256').update(await readFile(archive)).digest('hex') !== checksums[archiveName])
  throw new Error('Electron archive checksum mismatch');
await writeFile(path.join(stage, 'package.json'), JSON.stringify({ name: meta.name, version: meta.version,
  main: 'electron/main.cjs', productName: '本地拼音设置' }));
const result = await packager({ dir: stage, out: output, platform: 'win32', arch: 'x64',
  name: 'LocalPinyinSettings', executableName: 'LocalPinyinSettings', appVersion: meta.version,
  buildVersion: meta.version, electronVersion: meta.devDependencies.electron, asar: true,
  overwrite: true, prune: false, electronZipDir: archive ? path.dirname(archive) : undefined,
  download: { cacheRoot: cache, checksums },
  icon: path.join(workspace, 'assets/branding/icon/LocalPinyinIME.ico'),
  appCopyright: 'Copyright (c) 2026 LocalPinyinIME contributors',
  win32metadata: { CompanyName: 'LocalPinyinIME contributors', FileDescription: '本地拼音设置',
    OriginalFilename: 'LocalPinyinSettings.exe', ProductName: '本地拼音' } });
const product = checked(result[0]);
// The interface is Chinese; retain English as Chromium's fallback locale.
for (const name of await readdir(path.join(product, 'locales'))) {
  if (!['zh-CN.pak', 'en-US.pak'].includes(name)) await rm(path.join(product, 'locales', name));
}
await rename(path.join(product, 'LICENSE'), path.join(product, 'LICENSE.electron.txt'));
console.log(JSON.stringify({ desktopDirectory: product, electron: meta.devDependencies.electron, version: meta.version }));
