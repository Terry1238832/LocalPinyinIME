#!/usr/bin/env node
/* Real Electron + native bridge integration; no renderer or IPC mocks. */
'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const zlib = require('node:zlib');
const http = require('node:http');
const crypto = require('node:crypto');
const { spawnSync } = require('node:child_process');

const cli = Object.fromEntries(process.argv.slice(2).reduce((items, value, index, args) => {
  if (value.startsWith('--')) items.push([value.slice(2), args[index + 1]?.startsWith('--') ? true : args[index + 1] ?? true]);
  return items;
}, []));
const desktop = path.resolve(cli.app || path.join(__dirname, '..'));
const source = path.resolve(__dirname, '../..');
const expectedVersion = JSON.parse(fs.readFileSync(path.join(source, 'desktop/package.json'), 'utf8')).version;
const output = path.resolve(cli.output || path.join(source, '.build/electron-ui'));
const data = path.resolve(cli.data || path.join(output, `isolated-data-${Date.now()}`));
const bridge = path.resolve(cli.bridge || path.join(source, '.build/bin/LocalPinyinSettingsBridge.exe'));
const executable = path.resolve(cli.executable || path.join(desktop, 'node_modules/electron/dist/electron.exe'));
const { _electron } = require(process.env.LOCALPINYINIME_PLAYWRIGHT_MODULE || 'playwright');

fs.mkdirSync(output, { recursive: true });
assert(!fs.existsSync(data) || fs.readdirSync(data).length === 0, 'Choose a fresh --data directory; this suite never replaces existing data.');
fs.mkdirSync(data, { recursive: true });
assert(fs.existsSync(bridge), `Missing native bridge: ${bridge}`);
assert(fs.existsSync(executable), `Missing Electron executable: ${executable}`);

const report = { startedAt: new Date().toISOString(), app: desktop, executable, bridge, isolatedData: data, checks: [], screenshots: [], errors: [] };
let app;
let page;
let navigationCount = 0;

async function check(name, fn) {
  const start = performance.now();
  await fn();
  report.checks.push({ name, milliseconds: Math.round(performance.now() - start), passed: true });
  process.stdout.write(`PASS ${name}\n`);
}

async function launch(extra = []) {
  const environment = { ...process.env };
  delete environment.ELECTRON_RUN_AS_NODE;
  delete environment.LOCALPINYINIME_USER_LEXICON_PATH;
  delete environment.LOCALPINYINIME_DIAGNOSTICS;
  app = await _electron.launch({
    executablePath: executable,
    args: (cli.packaged ? [] : [desktop]).concat(extra),
    cwd: desktop,
    env: {
      ...environment,
      LOCALPINYINIME_BRIDGE_PATH: bridge,
      LOCALPINYINIME_QA_HEADLESS: '1',
      LOCALPINYINIME_LOCALAPPDATA: data,
    },
    timeout: 30000,
  });
  page = await app.firstWindow();
  page.setDefaultTimeout(10000);
  page.on('pageerror', error => report.errors.push(String(error)));
  page.on('framenavigated', frame => { if (frame === page.mainFrame()) navigationCount++; });
  await page.waitForFunction(() => !!window.localPinyin && document.querySelector('#page-appearance'));
  await page.getByRole('heading', { name: '外观', exact: true }).waitFor();
  await page.evaluate(() => window.localPinyin.getState());
  // Fresh data: the clipboard question comes first, once.
  const question = page.getByRole('dialog', { name: '要记住复制过的内容吗？', exact: true });
  if (await question.isVisible()) {
    report.clipboardQuestionShown = (report.clipboardQuestionShown ?? 0) + 1;
    await screenshot('clipboard-question');
    await question.getByRole('button', { name: '不用了', exact: true }).click();
    await question.waitFor({ state: 'detached' });
  }
  // Then, until it is answered once, the first-run welcome.
  const welcome = page.getByRole('dialog', { name: '欢迎使用本地拼音', exact: true });
  if (report.welcomeShown) assert.equal(await welcome.isVisible(), false, 'The welcome came back after it was answered.');
  else if (await welcome.waitFor({ timeout: 3000 }).then(() => true, () => false)) {
    report.welcomeShown = 1;
    await screenshot('welcome');
    await welcome.getByRole('button', { name: '以后再说', exact: true }).click();
    await welcome.waitFor({ state: 'detached' });
  }
}

async function state() { return page.evaluate(() => window.localPinyin.getState()); }
async function waitState(predicate) {
  for (let attempt = 0; attempt < 80; attempt++) {
    const value = await state();
    if (predicate(value)) return value;
    await new Promise(resolve => setTimeout(resolve, 50));
  }
  throw new Error('Native state did not reach the expected value.');
}
async function waitFontLevel(level) {
  await waitState(value => value.settings.textSize === level);
  await page.waitForFunction(expected => {
    const slider = document.querySelector('input[type="range"]');
    return slider && !slider.disabled && Number(slider.value) === expected;
  }, level);
}
async function navigate(id) {
  await page.locator('nav').getByRole('button', { name: id.label, exact: true }).click();
  await page.getByRole('heading', { name: id.heading, exact: true }).waitFor();
  assert.equal(await page.locator(`#page-${id.id}`).isVisible(), true);
}
const pages = [
  { id: 'appearance', label: '外观', heading: '外观' },
  { id: 'input', label: '输入', heading: '输入' },
  { id: 'dictionary', label: '词库', heading: '词库' },
  { id: 'learning', label: '学习', heading: '学习' },
  { id: 'about', label: '关于', heading: '关于' },
  // Third and fourth in the sidebar; last here so the indexes above stay put.
  { id: 'panel', label: '工具面板', heading: '工具面板' },
  { id: 'english', label: '英文', heading: '本地英文' },
];

// PNG decoding stays in the test, so no image package is required to reject blank captures.
function imageStats(bytes) {
  assert.equal(bytes.subarray(1, 4).toString(), 'PNG');
  let width, height, type, depth;
  const chunks = [];
  for (let cursor = 8; cursor < bytes.length;) {
    const size = bytes.readUInt32BE(cursor);
    const tag = bytes.subarray(cursor + 4, cursor + 8).toString();
    const payload = bytes.subarray(cursor + 8, cursor + 8 + size);
    if (tag === 'IHDR') {
      width = payload.readUInt32BE(0); height = payload.readUInt32BE(4); depth = payload[8]; type = payload[9];
      assert.equal(payload[12], 0, 'Screenshots must not be interlaced.');
    } else if (tag === 'IDAT') chunks.push(payload);
    cursor += size + 12;
  }
  assert.equal(depth, 8);
  assert([2, 6].includes(type), `Unexpected screenshot PNG type ${type}`);
  const channels = type === 6 ? 4 : 3;
  const stride = width * channels;
  const raw = zlib.inflateSync(Buffer.concat(chunks));
  const pixels = Buffer.alloc(stride * height);
  const paeth = (a, b, c) => {
    const p = a + b - c, da = Math.abs(p - a), db = Math.abs(p - b), dc = Math.abs(p - c);
    return da <= db && da <= dc ? a : db <= dc ? b : c;
  };
  for (let y = 0; y < height; y++) {
    const filter = raw[y * (stride + 1)];
    for (let x = 0; x < stride; x++) {
      const current = y * stride + x;
      const a = x >= channels ? pixels[current - channels] : 0;
      const b = y ? pixels[current - stride] : 0;
      const c = y && x >= channels ? pixels[current - stride - channels] : 0;
      const adjust = filter === 0 ? 0 : filter === 1 ? a : filter === 2 ? b : filter === 3 ? Math.floor((a + b) / 2) : paeth(a, b, c);
      pixels[current] = (raw[y * (stride + 1) + x + 1] + adjust) & 255;
    }
  }
  let white = 0, nearBlack = 0;
  const colours = new Set();
  for (let pixel = 0; pixel < width * height; pixel++) {
    const offset = pixel * channels;
    const r = pixels[offset], g = pixels[offset + 1], b = pixels[offset + 2];
    if (r >= 250 && g >= 250 && b >= 250) white++;
    if (r <= 3 && g <= 3 && b <= 3) nearBlack++;
    if (pixel % 17 === 0) colours.add((r << 16) | (g << 8) | b);
  }
  return { width, height, whiteFraction: white / (width * height), blackFraction: nearBlack / (width * height), sampledColours: colours.size };
}

async function screenshot(name) {
  const file = path.join(output, `${name}.png`);
  const bytes = await page.screenshot({ path: file, animations: 'disabled', style: '.data-path, .about-data-footer { visibility: hidden !important; }' });
  const stats = imageStats(bytes);
  assert(stats.whiteFraction < .94 && stats.blackFraction < .94 && stats.sampledColours > 40, `Blank frame: ${name} ${JSON.stringify(stats)}`);
  report.screenshots.push({ name, file, ...stats });
}

async function viewport(width, height) {
  await app.evaluate(({ BrowserWindow }, size) => BrowserWindow.getAllWindows()[0].setContentSize(size.width, size.height), { width, height });
  const metrics = await app.evaluate(({ BrowserWindow, screen }) => {
    const window = BrowserWindow.getAllWindows()[0];
    return { contentSize: window.getContentSize(), windowSize: window.getSize(), minimumSize: window.getMinimumSize(),
      scaleFactor: screen.getDisplayMatching(window.getBounds()).scaleFactor, platform: process.platform };
  });
  // Windows offscreen frameless windows at 150% DPI report 800 client DIP as
  // 801 renderer pixels, and the native 680 minimum clamps to 682. This is
  // stable native rounding, not delayed layout. Keep the tested width exact;
  // allow only the observed upward physical-pixel rounding in height.
  const heightRounding = metrics.platform === 'win32' ? Math.min(2, Math.ceil(metrics.scaleFactor)) : 0;
  await page.waitForFunction(size => innerWidth === size.width && innerHeight >= size.height &&
    innerHeight <= size.height + size.heightRounding, { width, height, heightRounding });
  const actual = await page.evaluate(() => ({ width: innerWidth, height: innerHeight, devicePixelRatio, zoom: visualViewport.scale }));
  (report.viewportMeasurements ||= []).push({ requested: { width, height }, actual, heightRounding, native: metrics });
}

async function noOverflow() {
  const measurements = await page.evaluate(() => {
    const main = document.querySelector('.main-content');
    return { width: innerWidth, scrollWidth: document.documentElement.scrollWidth, body: document.body.scrollWidth,
      mainWidth: main.clientWidth, mainScrollWidth: main.scrollWidth,
      cards: Array.from(document.querySelectorAll('.page:not([hidden]) .card, .page:not([hidden]) .live-preview, .page:not([hidden]) .paged-candidate-row')).map(element => {
        const bounds = element.getBoundingClientRect();
        const parent = element.parentElement.getBoundingClientRect();
        return { className: element.className, left: bounds.left, right: bounds.right,
          inside: bounds.left >= parent.left - 1 && bounds.right <= parent.right + 1 && bounds.right <= innerWidth + 1 };
      }) };
  });
  assert(measurements.scrollWidth <= measurements.width + 1 && measurements.body <= measurements.width + 1 &&
    measurements.mainScrollWidth <= measurements.mainWidth + 1 && measurements.cards.every(card => card.inside), `Horizontal overflow: ${JSON.stringify(measurements)}`);
}

async function api(name, args = []) {
  return page.evaluate(({ name, args }) => window.localPinyin[name](...args), { name, args });
}

function native(command, args = []) {
  const environment = { ...process.env, LOCALPINYINIME_LOCALAPPDATA: data };
  delete environment.LOCALPINYINIME_USER_LEXICON_PATH;
  delete environment.LOCALPINYINIME_DIAGNOSTICS;
  const result = spawnSync(bridge, [command, ...args], {
    env: environment, encoding: 'utf8', windowsHide: true, timeout: 20000, maxBuffer: 4 * 1024 * 1024,
  });
  assert.equal(result.status, 0, `Native ${command} failed: ${result.error?.message || result.stdout || result.stderr}`);
  return JSON.parse(result.stdout);
}

function savedFiles(directory, includeSettings = true) {
  const names = ['user_lexicon.tsv', 'user_learning.tsv', 'runtime_status.json'];
  if (includeSettings) names.push('settings.json');
  return Object.fromEntries(names.map(name => {
    const file = path.join(directory, name);
    return [name, fs.existsSync(file) ? fs.readFileSync(file).toString('base64') : null];
  }));
}

// The Appearance preview shows this fixed sample (previewSample in CandidatePreview.tsx).
const previewPinyin = "sui'bian";
const previewWords = ['随便', '岁', '随', '碎', '虽', '遂', '穗', '隋', '髓', '祟', '燧', '邃'];
const candidateStyleOrder = ['rail', 'type', 'glass', 'flow', 'capsule', 'column', 'flap', 'marker'];

async function waitPreviewCandidates(expected, snapshots) {
  await page.waitForFunction(words => {
    const panel = document.querySelector('[aria-label="候选预览"]');
    const actual = Array.from(panel?.querySelectorAll('.candidate-measurements .candidate-word') ?? []).map(element => element.textContent);
    return JSON.stringify(actual) === JSON.stringify(words);
  }, expected);
  if (expected.length) assert.match(await page.getByLabel('候选预览', { exact: true }).locator('.candidate-page-status').textContent(), /^1\//, 'A new font size or style did not reset the preview to page 1.');
  assert.deepEqual(await collectCandidatePages('候选预览', '.preview-candidate', snapshots), expected);
}

async function styleCardInView(style) {
  return page.locator(`.style-viewport .style-card:has(input[value="${style}"])`).evaluate(card => {
    const box = card.getBoundingClientRect();
    const view = card.closest('.style-viewport').getBoundingClientRect();
    return box.left >= view.left - 1 && box.right <= view.right + 1;
  });
}

async function chooseStyle(style) {
  // Turn to the card's group unless it is the current one, then let the
  // scroll settle before picking the card.
  const group = Math.floor(candidateStyleOrder.indexOf(style) / 4);
  const current = await page.locator('.carousel-dots i').evaluateAll(dots => dots.findIndex(dot => dot.classList.contains('on')));
  if (current !== group) await page.getByRole('button', { name: group > current ? '下一组风格' : '上一组风格', exact: true }).click();
  await page.waitForFunction(value => {
    const card = document.querySelector(`.style-viewport .style-card:has(input[value="${value}"])`);
    const box = card.getBoundingClientRect();
    const view = card.closest('.style-viewport').getBoundingClientRect();
    return box.left >= view.left - 1 && box.right <= view.right + 1;
  }, style);
  await page.locator(`.style-viewport .style-card:has(input[value="${style}"])`).click();
  return waitState(value => value.settings.candidateStyle === style);
}

async function collectCandidatePages(label, selector, snapshots) {
  // Font and viewport edits resize the measured row after React has committed the words.
  await page.evaluate(() => new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve))));
  const panel = page.getByLabel(label, { exact: true });
  const previous = panel.getByRole('button', { name: `${label}上一页`, exact: true });
  const next = panel.getByRole('button', { name: `${label}下一页`, exact: true });
  if (!(await panel.locator('.paged-candidate-items ' + selector).count())) return [];
  // Like the native popup, the pager ("<  2/5  >") only appears with several pages.
  const pager = panel.locator('.candidate-page-controls .candidate-pager');
  const pageTotal = Number((await panel.locator('.candidate-page-status').textContent()).split('/')[1]);
  assert.equal(await pager.isVisible(), pageTotal > 1, `${label}: pager visibility does not match ${pageTotal} page(s)`);
  const enabled = async button => (await pager.isVisible()) && await button.isEnabled();
  for (let i = 0; i < 64 && await enabled(previous); ++i) {
    const current = Number((await panel.locator('.candidate-page-status').textContent()).split('/')[0]);
    await previous.click();
    await page.waitForFunction(({ label, number }) => document.querySelector(`[aria-label="${label}"] .candidate-page-status`)?.textContent.startsWith(`${number}/`), { label, number: current - 1 });
  }
  assert(!(await enabled(previous)), `${label}: pagination did not return to first page`);
  const result = [];
  for (let i = 0; i < 64; ++i) {
    const items = panel.locator('.paged-candidate-items ' + selector);
    const measurements = await items.evaluateAll(elements => elements.map(element => {
      const bounds = element.getBoundingClientRect();
      const parent = element.closest('.paged-candidate-items').getBoundingClientRect();
      return { text: element.querySelector('.candidate-word')?.textContent || element.firstChild?.textContent, title: element.title,
        number: element.querySelector('.candidate-number')?.textContent, top: bounds.top, bottom: bounds.bottom,
        inside: bounds.left >= parent.left - 1 && bounds.right <= parent.right + 1 };
    }));
    assert(measurements.length, `${label}: page ${i + 1} has no candidates`);
    assert(measurements.every(item => item.inside), `${label}: candidate overflow`);
    // The preview follows its style: one row (Cursor Rail, Modern Type,
    // Floating Glass, Capsule, Split-Flap, Highlighter, Adaptive Flow's Compact form),
    // the current choice above one row of chips (Balanced), or one candidate
    // per row (Expanded, Column List). The suggestion and context lanes are a
    // single row.
    const arrangement = selector === '.preview-candidate'
      ? await page.evaluate(() => document.querySelector('.ime-preview')?.dataset.arrangement) : 'row';
    const sameRow = items => items.length < 2 || Math.max(...items.map(item => item.top)) < Math.min(...items.map(item => item.bottom)) - 1;
    if (arrangement === 'row') assert(sameRow(measurements), `${label}: more than one row`);
    else if (arrangement === 'focus-row') {
      const chips = measurements.slice(1);
      assert(chips.every(item => Math.abs(item.top - chips[0].top) <= 1 && Math.abs(item.bottom - chips[0].bottom) <= 1), `${label}: chips on more than one row`);
      if (chips.length) assert(chips[0].top >= measurements[0].bottom - 1, `${label}: focus row is not above the chips`);
    } else {
      assert.equal(arrangement, 'column', `${label}: unknown arrangement`);
      assert(measurements.every((item, index) => !index || item.top >= measurements[index - 1].bottom - 1), `${label}: rows overlap`);
    }
    assert(measurements.every(item => item.title === item.text), `${label}: displayed value differs from complete candidate`);
    if (selector === '.preview-candidate') assert.deepEqual(measurements.map(item => item.number), measurements.map((_, index) => String(index + 1)), `${label}: page numbers do not restart at 1`);
    const status = await panel.locator('.candidate-page-status').textContent();
    assert.equal(Number(status.split('/')[0]), i + 1, `${label}: incorrect page status`);
    if (snapshots) snapshots.push({ status, candidates: measurements.map(item => item.text) });
    result.push(...measurements.map(item => item.text));
    if (!(await enabled(next))) break;
    await next.click();
    await page.waitForFunction(({ label, number }) => document.querySelector(`[aria-label="${label}"] .candidate-page-status`)?.textContent.startsWith(`${number}/`), { label, number: i + 2 });
  }
  assert(!(await enabled(next)), `${label}: pagination did not reach last page`);
  for (let i = 0; i < 64 && await enabled(previous); ++i) {
    const current = Number((await panel.locator('.candidate-page-status').textContent()).split('/')[0]);
    await previous.click();
    await page.waitForFunction(({ label, number }) => document.querySelector(`[aria-label="${label}"] .candidate-page-status`)?.textContent.startsWith(`${number}/`), { label, number: current - 1 });
  }
  assert(!(await enabled(previous)), `${label}: pagination did not return to first page`);
  return result;
}

const feed = { version: null, installer: Buffer.alloc(0), checksum: '', requests: [] };
const sha256 = buffer => crypto.createHash('sha256').update(buffer).digest('hex');
const feedServer = http.createServer((request, response) => {
  feed.requests.push(request.url);
  const base = `http://127.0.0.1:${feedServer.address().port}`;
  if (request.url.startsWith('/releases')) {
    const releases = [{ tag_name: `v${expectedVersion}`, draft: false, html_url: `${base}/release/v${expectedVersion}`, body: '',
      published_at: '2026-10-18T00:00:00Z', assets: [] }];
    if (feed.version) releases.unshift({ tag_name: `v${feed.version}`, draft: false, prerelease: true,
      html_url: `${base}/release/v${feed.version}`, published_at: '2026-10-20T08:00:00Z',
      body: '## 测试版本\n\n- **新功能**：自动检测更新\n- 修复若干问题',
      assets: [{ name: `LocalPinyinIME-${feed.version}-x64.exe`, size: feed.installer.length, browser_download_url: `${base}/download/installer.exe` },
        { name: `LocalPinyinIME-${feed.version}-x64.exe.sha256`, size: 97, browser_download_url: `${base}/download/installer.exe.sha256` }] });
    response.writeHead(200, { 'Content-Type': 'application/json' });
    response.end(JSON.stringify(releases));
  } else if (request.url === '/download/installer.exe') {
    response.writeHead(200, { 'Content-Type': 'application/octet-stream', 'Content-Length': feed.installer.length });
    response.end(feed.installer);
  } else if (request.url === '/download/installer.exe.sha256') {
    response.writeHead(200, { 'Content-Type': 'text/plain' });
    response.end(`${feed.checksum}  LocalPinyinIME-${feed.version}-x64.exe\n`);
  } else {
    response.writeHead(404);
    response.end();
  }
});

async function run() {
  await new Promise(resolve => feedServer.listen(0, '127.0.0.1', resolve));
  process.env.LOCALPINYINIME_UPDATE_FEED = `http://127.0.0.1:${feedServer.address().port}/releases?per_page=30`;
  process.env.LOCALPINYINIME_UPDATE_DRY_RUN = '1';
  process.env.LOCALPINYINIME_QA_BACKUP_FILE = path.join(output, 'backup.lpbackup');
  process.env.LOCALPINYINIME_NO_AUTOSTART = '1';
  fs.rmSync(process.env.LOCALPINYINIME_QA_BACKUP_FILE, { force: true });
  // The first start is the installer's: --first-run, which must open the
  // window with the clipboard question (0.24.2 left it closed).
  await launch(['--first-run']);
  await check('The clipboard question is asked once on a fresh install and answered', async () => {
    assert.equal(report.clipboardQuestionShown, 1);
    assert.equal((await state()).settings.clipboardHistory, 'off');
  });
  await check('The first-run welcome is shown once; About replays the tour on the real pages', async () => {
    assert.equal(report.welcomeShown, 1);
    await navigate(pages.find(item => item.id === 'about'));
    await page.locator('#page-about').getByRole('button', { name: '重新观看', exact: true }).click();
    await page.getByRole('dialog', { name: '欢迎使用本地拼音', exact: true }).getByRole('button', { name: /开始导览/ }).click();
    const card = page.locator('.tour-card');
    const steps = [['挑一个候选窗口', 'appearance'], ['中、英一键切换', 'input'], ['第二个输入法：本地英文', 'english'],
      ['连按两下 Tab：工具面板', 'panel'], ['找设置，按 Ctrl + K', 'panel']];
    for (const [index, [title, id]] of steps.entries()) {
      await card.getByRole('heading', { name: title, exact: true }).waitFor();
      assert.equal(await page.locator(`#page-${id}`).isVisible(), true);
      assert.equal(await card.locator('[aria-current="step"]').textContent(), String(index + 1));
      if (index === 1) await screenshot('tour-step');
      await card.getByRole('button', { name: index === steps.length - 1 ? '完成' : '下一步', exact: true }).click();
    }
    await card.waitFor({ state: 'detached' });
    assert.equal(await page.evaluate(() => localStorage.getItem('localpinyin.tour')), 'done');
    await navigate(pages.find(item => item.id === 'appearance'));
  });
  await check('Real native state and preview', async () => {
    const initial = await state();
    assert.equal(initial.version, expectedVersion);
    assert.equal(initial.settings.predictionEnabled, false, 'Next-word suggestions are opt-in on fresh installations.');
    assert.equal(initial.dictionary.total, 259380, `The ${expectedVersion} built-in resources were not loaded.`);
    report.dictionary = initial.dictionary;
    assert.equal(path.resolve(initial.dataDirectory), path.join(data, 'LocalPinyinIME'));
    assert.deepEqual((await api('listUserLexicon')).entries, []);
    assert.equal(native('lookup', ['suibian']).candidates[0], '随便');
    for (const context of ['', '请帮忙']) {
      assert(native('lookup', ['jinyibufenxi', context]).candidates.includes('进一步分析'),
        'A complete-key common expression was lost among segmented alternatives.');
    }
    assert.deepEqual(native('lookup', ['']).candidates, []);
    // The Appearance preview only shows a fixed sample: no input field, no lookups.
    assert.equal(await page.getByLabel('试试拼音', { exact: true }).count(), 0);
    assert.equal(await page.locator('.ime-preview-pinyin').textContent(), previewPinyin);
    await waitPreviewCandidates(previewWords);
    await screenshot('appearance-preview');
  });
  await check('Electron renderer isolation and fixed bridge', async () => {
    const exposed = await page.evaluate(() => ({ require: typeof window.require, process: typeof window.process, api: Object.keys(window.localPinyin).sort(), frozen: Object.isFrozen(window.localPinyin) }));
    assert.equal(exposed.require, 'undefined');
    assert.equal(exposed.process, 'undefined');
    assert.equal(exposed.frozen, true);
    assert.deepEqual(exposed.api, ['addPhrase', 'checkForUpdate', 'chooseBackupFile', 'clearClipboard', 'clearLearning', 'clipboardSummary',
      'deleteEmojiCandidate', 'deletePhrase', 'deleteUserEntry', 'domainWords', 'downloadUpdate', 'exportBackup', 'getState',
      'forgetLearning', 'getUpdateStatus', 'importBackup', 'initialState', 'inspectBackup', 'installUpdate', 'listEmojiCandidates', 'listLearning', 'listPhrases',
      'listUserLexicon', 'onSettingsChanged', 'onUpdateStatus', 'openDataDirectory', 'openReleasePage',
      'saveSettings', 'searchDictionary', 'setAutoUpdateCheck', 'upsertUserEntry', 'vaultChangePassword', 'vaultCreate', 'vaultDestroy',
      'vaultStatus', 'windowControl'].sort());
    assert.equal(await page.evaluate(() => typeof window.localPinyin.initialState), 'object');
    assert.equal(await page.evaluate(() => window.localPinyin.initialState.version), expectedVersion);
    const preferences = await app.evaluate(({ BrowserWindow }) => BrowserWindow.getAllWindows()[0].webContents.getLastWebPreferences());
    assert.equal(preferences.nodeIntegration, false);
    assert.equal(preferences.contextIsolation, true);
    assert.equal(preferences.sandbox, true);
    assert.equal(preferences.webSecurity, true);
    const windowState = await app.evaluate(({ app, BrowserWindow }) => ({ packaged: app.isPackaged,
      visible: BrowserWindow.getAllWindows()[0].isVisible(), focused: BrowserWindow.getAllWindows()[0].isFocused(),
      offscreen: BrowserWindow.getAllWindows()[0].webContents.isOffscreen(),
      backgroundThrottling: BrowserWindow.getAllWindows()[0].webContents.getBackgroundThrottling() }));
    assert.equal(windowState.offscreen, true);
    assert.equal(windowState.backgroundThrottling, false);
    assert.equal(windowState.visible, false);
    assert.equal(windowState.focused, false);
    if (cli.packaged) assert.equal(windowState.packaged, true);
    report.security = { contextIsolation: true, sandbox: true, fixedIpc: true, offscreen: true,
      hiddenWindow: true, focused: false, packaged: windowState.packaged, isolatedPersonalData: (await state()).dataDirectory };
  });
  await check('Rapid side navigation preserves shell and all page state', async () => {
    const beforeNavigation = navigationCount;
    const observations = await page.evaluate(async () => {
      const shell = document.querySelector('#root')?.firstElementChild;
      const nav = document.querySelector('nav');
      const sections = Array.from(document.querySelectorAll('section[id^="page-"]'));
      const expected = ['appearance', 'input', 'english', 'panel', 'dictionary', 'learning', 'about'];
      const buttons = Array.from(nav.querySelectorAll('button'));
      const failures = [];
      let frames = 0;
      const nextFrame = () => new Promise((resolve, reject) => {
        const timeout = setTimeout(() => reject(new Error(`Hidden-window RAF did not paint (visibility=${document.visibilityState}); enable QA offscreen rendering.`)), 1500);
        requestAnimationFrame(() => { clearTimeout(timeout); resolve(); });
      });
      for (let iteration = 0; iteration < 80; iteration++) {
        const index = iteration % expected.length;
        buttons[index].click();
        await nextFrame(); await nextFrame();
        frames += 2;
        const visible = sections.filter(section => !section.hidden);
        if (document.querySelector('#root')?.firstElementChild !== shell || document.querySelector('nav') !== nav) failures.push('shell replaced');
        if (visible.length !== 1 || visible[0].id !== `page-${expected[index]}`) failures.push(`wrong visible page ${iteration}`);
        if (!visible[0]?.querySelector('h1')?.textContent.trim()) failures.push(`blank title ${iteration}`);
        if (sections.some(section => !section.isConnected)) failures.push('page unmounted');
        if (buttons.filter(button => button.getAttribute('aria-current') === 'page').length !== 1) failures.push('navigation selection lost');
      }
      return { failures, frames, sections: sections.length, samePreview: document.querySelector('.ime-preview-pinyin')?.textContent === "sui'bian" };
    });
    assert.deepEqual(observations.failures, []);
    assert.equal(observations.sections, 7);
    assert.equal(observations.samePreview, true);
    assert.equal(navigationCount, beforeNavigation, 'Clicking sidebar reloaded the document.');
    report.navigationFrames = observations.frames;
    report.navigationTransitions = observations.frames / 2;
    for (const item of pages) { await navigate(item); await screenshot(`navigation-${item.id}`); }
  });
  await check('Light/dark and minimum-window layout', async () => {
    for (const theme of ['light', 'dark']) {
      await navigate(pages[0]);
      await page.getByRole('radio', { name: theme === 'light' ? '浅色' : '深色', exact: true }).locator('..').click();
      await waitState(value => value.settings.theme === theme);
      for (const size of [[1160, 800], [920, 680]]) {
        await viewport(...size);
        for (const item of pages) {
          await navigate(item); await noOverflow(); await screenshot(`${theme}-${size[0]}-${item.id}`);
        }
      }
    }
    await viewport(1160, 800);
  });
  await check('Native persistence through UI and process restart', async () => {
    await navigate(pages[0]);
    const slider = page.getByRole('slider');
    await slider.focus();
    await page.keyboard.press('End');
    const fontValue = Number(await slider.inputValue());
    assert.equal((await waitState(value => value.settings.textSize === fontValue)).settings.theme, 'dark');
    await navigate(pages[1]);
    // The candidate style is saved from its card and kept on its own.
    await navigate(pages[0]);
    const beforeStyle = (await state()).settings;
    await page.locator('.style-card input[value="glass"]').check({ force: true });
    const styled = await waitState(value => value.settings.candidateStyle === 'glass');
    assert.deepEqual(styled.settings, { ...beforeStyle, candidateStyle: 'glass' });
    await navigate(pages[1]);
    const expectedShift = !(await state()).settings.shiftToggleEnabled;
    const shift = page.getByRole('switch', { name: '按 Shift 切换中英文', exact: true });
    await shift.click();
    await waitState(value => value.settings.shiftToggleEnabled === expectedShift);
    const saved = (await state()).settings;
    await app.close();
    await launch();
    assert.deepEqual((await state()).settings, saved);
    assert.deepEqual(await page.evaluate(() => window.localPinyin.initialState.settings), saved);
    await navigate(pages[1]);
    assert.equal(await page.getByRole('switch', { name: '按 Shift 切换中英文', exact: true }).getAttribute('aria-checked'), String(saved.shiftToggleEnabled));
    assert.equal(saved.candidateStyle, 'glass');
    await navigate(pages[0]);
    assert.equal(await page.locator('.ime-preview').getAttribute('data-cand-style'), 'glass');
    assert.equal(await page.locator('.style-card input[value="glass"]').isChecked(), true);
    // Back to the default style for the checks that follow.
    await page.locator('.style-card input[value="flow"]').check({ force: true });
    await waitState(value => value.settings.candidateStyle === 'flow');
    report.persistedSettings = saved;
  });
  await check('The preview pages its whole sample across sizes and keyboard boundaries', async () => {
    await navigate(pages[0]);
    const initial = await state();
    const originalTextSize = initial.settings.textSize;
    const filesBefore = savedFiles(initial.dataDirectory);
    const previewPanel = page.getByLabel('候选预览', { exact: true });
    const previewRow = previewPanel.getByRole('group', { name: '候选预览，左右箭头翻页', exact: true });
    const slider = page.getByRole('slider');
    const matrix = [];
    for (const [width, height] of [[1160, 800], [920, 680]]) {
      await viewport(width, height);
      await slider.focus(); await page.keyboard.press('Home');
      await waitFontLevel(1);
      for (let level = 1; level <= 7; ++level) {
        if (level > 1) {
          await slider.focus(); await page.keyboard.press('ArrowRight');
          await waitFontLevel(level);
        }
        const fontSize = [14, 16, 17, 18, 20, 22, 24][level - 1];
        await page.waitForFunction(expected => getComputedStyle(document.querySelector('[aria-label="候选预览"]')).fontSize === `${expected}px`, fontSize);
        const snapshots = [];
        await waitPreviewCandidates(previewWords, snapshots);
        await noOverflow();
        matrix.push({ width, fontSize, pages: snapshots });
      }
    }
    const pageCount = Number((await previewPanel.locator('.candidate-page-status').textContent()).split('/')[1]);
    assert(pageCount > 1, 'The sample holds more than one page of candidates.');
    await screenshot('paging-preview-page-1');
    await previewRow.focus();
    await page.keyboard.press('ArrowLeft');
    assert.equal(await previewPanel.locator('.candidate-page-status').textContent(), `1/${pageCount}`);
    await page.keyboard.press('ArrowRight');
    assert.equal(await previewPanel.locator('.candidate-page-status').textContent(), `2/${pageCount}`);
    assert.equal(await previewPanel.locator('.paged-candidate-items .candidate-number').first().textContent(), '1');
    await screenshot('paging-preview-page-2');
    for (let index = 0; index < pageCount + 2; ++index) await page.keyboard.press('ArrowRight');
    assert.equal(await previewPanel.locator('.candidate-page-status').textContent(), `${pageCount}/${pageCount}`);
    assert.equal(await previewPanel.getByRole('button', { name: '候选预览下一页', exact: true }).isDisabled(), true);
    for (let index = 0; index < pageCount + 2; ++index) await page.keyboard.press('ArrowLeft');
    assert.equal(await previewPanel.locator('.candidate-page-status').textContent(), `1/${pageCount}`);
    assert.equal(await previewPanel.getByRole('button', { name: '候选预览上一页', exact: true }).isDisabled(), true);

    // Back to the saved size; previewing and paging leave every file unchanged.
    await slider.focus(); await page.keyboard.press('Home');
    await waitFontLevel(1);
    for (let level = 2; level <= originalTextSize; ++level) {
      await slider.focus(); await page.keyboard.press('ArrowRight');
      await waitFontLevel(level);
    }
    assert.deepEqual(savedFiles(initial.dataDirectory), filesBefore, 'Paging the preview changed personal files.');
    await viewport(1160, 800);
    report.pagination = { sample: previewWords, matrix, keyboardClamped: true, previewFilesUnchanged: true };
  });
  await check('Eight candidate styles: the carousel turns, 更多 shows all, each choice persists for the IME', async () => {
    await navigate(pages[0]);
    const before = await state();
    const settingsFile = path.join(before.dataDirectory, 'settings.json');
    const previous = page.getByRole('button', { name: '上一组风格', exact: true });
    const next = page.getByRole('button', { name: '下一组风格', exact: true });
    // Four cards in view, the first group; the arrows turn to the other four.
    assert.equal(await page.locator('.style-viewport .style-card').count(), candidateStyleOrder.length);
    const inView = async () => (await Promise.all(candidateStyleOrder.map(styleCardInView))).map((shown, index) => shown ? candidateStyleOrder[index] : null).filter(Boolean);
    assert.deepEqual(await inView(), candidateStyleOrder.slice(0, 4));
    assert.equal(await previous.isDisabled(), true);
    await next.click();
    await page.waitForFunction(() => document.querySelector('.carousel-dots i:last-child')?.classList.contains('on'));
    await page.waitForFunction(style => {
      const card = document.querySelector(`.style-viewport .style-card:has(input[value="${style}"])`).getBoundingClientRect();
      return Math.abs(card.left - document.querySelector('.style-viewport').getBoundingClientRect().left) < 6;
    }, candidateStyleOrder[4]);
    assert.deepEqual(await inView(), candidateStyleOrder.slice(4));
    assert.equal(await next.isDisabled(), true);
    await screenshot('style-carousel-second');
    await previous.click();
    await page.waitForFunction(() => document.querySelector('.carousel-dots i:first-child')?.classList.contains('on'));
    const styles = {};
    for (const style of candidateStyleOrder) {
      const saved = await chooseStyle(style);
      // Only the style changes: theme, size and input options stay.
      assert.deepEqual(saved.settings, { ...before.settings, candidateStyle: style });
      // A fresh native process, as the IME reads it on the next key.
      assert.equal(native('state').settings.candidateStyle, style);
      assert.match(fs.readFileSync(settingsFile, 'utf8'), new RegExp(`"style": "${style}"`));
      assert.equal(await page.locator('.ime-preview').getAttribute('data-cand-style'), style);
      assert.equal(await page.locator('.style-viewport .style-card.active input').getAttribute('value'), style);
      const snapshots = [];
      await waitPreviewCandidates(previewWords, snapshots);
      await noOverflow();
      styles[style] = { arrangement: await page.locator('.ime-preview').getAttribute('data-arrangement'),
        form: await page.locator('.ime-preview').getAttribute('data-form'), pages: snapshots.length };
      await screenshot(`style-${style}`);
    }
    // Column List stacks its rows and keeps the fullest page's height.
    assert.equal(styles.column.arrangement, 'column');
    for (const style of ['capsule', 'flap', 'marker']) assert.equal(styles[style].arrangement, 'row');
    // 更多: all eight at once, four to a row; a pick there applies at once.
    await page.getByRole('button', { name: '更多', exact: true }).click();
    const overview = page.getByRole('dialog', { name: '全部候选框风格', exact: true });
    await overview.waitFor();
    const cards = overview.locator('.style-card');
    assert.equal(await cards.count(), candidateStyleOrder.length);
    const tops = await cards.evaluateAll(elements => elements.map(element => Math.round(element.getBoundingClientRect().top)));
    assert.equal(new Set(tops.slice(0, 4)).size, 1, 'The first four styles share a row.');
    assert.equal(new Set(tops.slice(4)).size, 1, 'The last four styles share a row.');
    assert(tops[4] > tops[0], 'The second four sit below the first.');
    assert.equal(await overview.locator('.style-card.active input').getAttribute('value'), 'marker');
    await screenshot('style-overview');
    await overview.locator('.style-card:has(input[value="glass"])').click();
    await waitState(value => value.settings.candidateStyle === 'glass');
    assert.equal(await overview.locator('.style-card.active input').getAttribute('value'), 'glass');
    await page.keyboard.press('Escape');
    await overview.waitFor({ state: 'detached' });
    // The gallery turns to the chosen style's group.
    await page.waitForFunction(() => document.querySelector('.carousel-dots i:first-child')?.classList.contains('on'));
    await page.waitForFunction(() => {
      const card = document.querySelector('.style-viewport .style-card.active')?.getBoundingClientRect();
      const view = document.querySelector('.style-viewport').getBoundingClientRect();
      return card && card.left >= view.left - 1 && card.right <= view.right + 1;
    });
    // Cursor Rail, Modern Type, the Highlighter and Adaptive Flow's drop move
    // their mark to the candidate under the pointer.
    for (const [style, selector] of [['rail', '.rail-marker'], ['type', '.type-rule'], ['marker', '.marker-stroke'], ['flow', '.flow-drop']]) {
      await chooseStyle(style);
      const marker = page.locator(selector);
      await marker.waitFor();
      const rest = await marker.boundingBox();
      await page.locator('.paged-candidate-items [data-slot="2"]').hover();
      // The mark glides (Flow's drop flows, ~0.5 s) and settles under the hovered candidate.
      await page.waitForFunction(selector => {
        const mark = document.querySelector(selector).getBoundingClientRect();
        const target = document.querySelector('.paged-candidate-items [data-slot="2"]').getBoundingClientRect();
        return mark.left >= target.left - 1 && mark.right <= target.right + 1 && mark.width > 4;
      }, selector);
      const moved = await marker.boundingBox();
      assert(Math.abs(moved.x - rest.x) > 8, `${style}: focus mark did not follow the pointer`);
      await page.mouse.move(2, 2);
    }
    await chooseStyle('flow');
    report.candidateStyles = styles;
  });
  await check('Spelling correction, mixed input, fuzzy pinyin and double pinyin save and drive the native engine', async () => {
    await navigate(pages[1]);
    const before = await state();
    assert.deepEqual([before.settings.typoCorrection, before.settings.mixedEnglish, before.settings.fuzzyPinyin, before.settings.doublePinyin],
      [true, true, [], 'none'], 'Defaults: correction and mixed input on, no fuzzy pairs, full pinyin.');
    const idle = () => page.waitForFunction(() => !document.querySelector('.segmented input')?.disabled);
    // Settings are rows: switches on the right, fuzzy pinyin behind its row.
    const fuzzyRow = page.getByRole('button', { name: /^模糊音/ });
    await fuzzyRow.scrollIntoViewIfNeeded();
    assert.equal(await fuzzyRow.getAttribute('aria-expanded'), 'false');
    assert.match(await fuzzyRow.textContent(), /关闭/);
    await fuzzyRow.click();
    assert.equal(await fuzzyRow.getAttribute('aria-expanded'), 'true');
    // Fuzzy pairs: each chip saves on its own and the engine reads zongguo as 中国.
    assert.notEqual(native('lookup', ['zongguo']).candidates[0], '中国');
    await page.getByRole('button', { name: 'z 与 zh 不区分', exact: true }).click();
    await waitState(value => value.settings.fuzzyPinyin.join() === 'z-zh');
    await idle();
    await page.getByRole('button', { name: 'in 与 ing 不区分', exact: true }).click();
    const fuzzy = await waitState(value => value.settings.fuzzyPinyin.join() === 'z-zh,in-ing');
    assert.deepEqual({ ...fuzzy.settings, fuzzyPinyin: [] }, { ...before.settings, fuzzyPinyin: [] }, 'A fuzzy pair changed another setting.');
    assert.equal(await page.getByRole('button', { name: 'z 与 zh 不区分', exact: true }).getAttribute('aria-pressed'), 'true');
    assert.deepEqual(native('state').settings.fuzzyPinyin, ['z-zh', 'in-ing']);
    assert.equal(native('lookup', ['zongguo']).candidates[0], '中国');
    assert.match(await fuzzyRow.textContent(), /已开启 2 项/);
    // Spelling correction off: nihso no longer becomes 你好.
    assert.equal(native('lookup', ['nihso']).candidates[0], '你好');
    await idle();
    await page.getByRole('switch', { name: '拼写纠错', exact: true }).click();
    await waitState(value => value.settings.typoCorrection === false);
    assert.notEqual(native('lookup', ['nihso']).candidates[0], '你好');
    await idle();
    await page.getByRole('switch', { name: '中英混输', exact: true }).click();
    await waitState(value => value.settings.mixedEnglish === false);
    assert(!native('lookup', ['ppt']).candidates.includes('PPT'));
    // Double pinyin shows its key map and changes the Appearance preview.
    await idle();
    await page.locator('.segmented input[value="xiaohe"]').click({ force: true });
    await waitState(value => value.settings.doublePinyin === 'xiaohe');
    await page.getByLabel('小鹤双拼键位', { exact: true }).waitFor();
    await screenshot('input-options-xiaohe');
    assert.equal(native('lookup', ['nihcuijp']).candidates[0], '你好世界');
    assert.equal(native('lookup', ['nihcuijp']).pinyin, "ni'hao'shi'jie");
    // The options survive a restart, then return to their defaults.
    const saved = (await state()).settings;
    await app.close();
    await launch();
    assert.deepEqual((await state()).settings, saved);
    await navigate(pages[1]);
    await page.locator('#group-scheme').scrollIntoViewIfNeeded();
    assert.equal(await page.locator('.segmented input[value="xiaohe"]').isChecked(), true);
    // Back to the defaults through the same controls.
    await page.locator('.segmented input[value="none"]').click({ force: true });
    await waitState(value => value.settings.doublePinyin === 'none');
    assert.equal(await page.getByLabel('小鹤双拼键位', { exact: true }).count(), 0);
    await idle();
    await page.getByRole('switch', { name: '拼写纠错', exact: true }).click();
    await waitState(value => value.settings.typoCorrection === true);
    await idle();
    await page.getByRole('switch', { name: '中英混输', exact: true }).click();
    await waitState(value => value.settings.mixedEnglish === true);
    await idle();
    await page.getByRole('button', { name: /^模糊音/ }).click();
    await page.getByRole('button', { name: '全部关闭', exact: true }).click();
    await waitState(value => value.settings.fuzzyPinyin.length === 0);
    assert.deepEqual((await state()).settings, before.settings);
    report.inputOptions = { fuzzy: ['z-zh', 'in-ing'], correctionOff: true, mixedOff: true, doublePinyin: 'xiaohe', restored: true };
  });
  await check('Usage scenes save, list their words and drive the native engine', async () => {
    await navigate(pages[1]);
    const before = await state();
    assert.deepEqual(before.settings.usageDomains, [], 'No usage scene is chosen by default.');
    const idle = () => page.waitForFunction(() => !document.querySelector('.scene-card')?.disabled);
    const daily = page.getByRole('button', { name: /^日常聊天/ });
    const coding = page.getByRole('button', { name: /^编程开发/ });
    await daily.scrollIntoViewIfNeeded();
    assert.equal(await daily.getAttribute('aria-pressed'), 'false');
    assert.equal(native('lookup', ['haoxiangshi']).candidates[0], '好象是');
    // Each card saves on its own and the engine ranks the scene's words first.
    await daily.click();
    await waitState(value => value.settings.usageDomains.join() === 'daily');
    assert.equal(await daily.getAttribute('aria-pressed'), 'true');
    assert.equal(native('lookup', ['haoxiangshi']).candidates[0], '好像是');
    await idle();
    await coding.click();
    const chosen = await waitState(value => value.settings.usageDomains.join() === 'coding,daily');
    assert.deepEqual({ ...chosen.settings, usageDomains: [] }, before.settings, 'A scene changed another setting.');
    assert.equal(native('lookup', ['yilaizhuru']).candidates[0], '依赖注入');
    // The chosen scenes' words are listed behind their row.
    const words = page.getByRole('button', { name: /^场景词条/ });
    await page.waitForFunction(() => /2 个场景 · \d+ 个词/.test(document.querySelector('[aria-labelledby="group-scenes"]')?.textContent ?? ''));
    await words.click();
    assert.equal(await words.getAttribute('aria-expanded'), 'true');
    const panel = page.getByRole('region', { name: '场景词条' });
    assert.match(await panel.textContent(), /其实/);
    assert.match(await panel.textContent(), /依赖注入/);
    await screenshot('input-scenes');
    // Back to none through the same cards.
    await idle();
    await daily.click();
    await waitState(value => value.settings.usageDomains.join() === 'coding');
    await idle();
    await coding.click();
    await waitState(value => value.settings.usageDomains.length === 0);
    assert.match(await panel.textContent(), /先在上面选择场景/);
    assert.equal(native('lookup', ['haoxiangshi']).candidates[0], '好象是');
    assert.deepEqual((await state()).settings, before.settings);
    report.usageScenes = { chosen: ['coding', 'daily'], listed: true, restored: true };
  });
  await check('Typing helpers switch off and on and persist for the IME', async () => {
    await navigate(pages[1]);
    const before = await state();
    const helpers = [['digitPeriod', '数字后打。变成 .', true], ['digitPunctuation', '数字后使用英文标点', true],
      ['letterPunctuation', '字母后使用英文标点', false], ['codePunctuation', '编程软件中使用英文标点', true]];
    assert.deepEqual(helpers.map(([key]) => before.settings[key]), helpers.map(([, , on]) => on),
      'Digit and code helpers are on by default, letters off.');
    const idle = () => page.waitForFunction(() => !document.querySelector('.segmented input')?.disabled);
    // Each switch saves on its own and only changes its own setting.
    for (const [key, label, on] of helpers) {
      const toggle = page.getByRole('switch', { name: label, exact: true });
      await toggle.scrollIntoViewIfNeeded();
      await idle();
      await toggle.click();
      const changed = await waitState(value => value.settings[key] === !on);
      assert.deepEqual({ ...changed.settings, [key]: on }, before.settings, `${key} changed another setting.`);
      assert.equal(native('state').settings[key], !on, `${key} is saved for the IME.`);
      if (key === 'letterPunctuation') await screenshot('input-helpers');
      await idle();
      await toggle.click();
      await waitState(value => value.settings[key] === on);
    }
    assert.deepEqual((await state()).settings, before.settings);
    report.typingHelpers = { toggled: helpers.map(([key]) => key), restored: true };
  });
  await check('Setting explanations stay closed until their details button opens them', async () => {
    await navigate(pages[1]);
    assert.equal(await page.getByText('试一试', { exact: true }).count(), 0, 'The 试一试 panel is gone.');
    const visibleNotes = await page.evaluate(() => Array.from(document.querySelectorAll('#page-input .setting-detail'))
      .filter(element => element.getClientRects().length > 0).length);
    assert.equal(visibleNotes, 0, 'No explanation is open before it is asked for.');
    const details = page.getByRole('button', { name: '拼写纠错详情', exact: true });
    await details.scrollIntoViewIfNeeded();
    assert.equal(await details.getAttribute('aria-expanded'), 'false');
    await details.click();
    assert.equal(await details.getAttribute('aria-expanded'), 'true');
    const panel = page.getByRole('region', { name: '拼写纠错说明', exact: true });
    assert.match(await panel.textContent(), /nihso/);
    assert.equal(await panel.locator('.typing-demo').count(), 1, 'The explanation shows its demonstration.');
    await page.waitForTimeout(2600);
    await screenshot('details-typing-demo');
    const keyDetails = page.getByRole('button', { name: '数字后使用英文标点详情', exact: true });
    await keyDetails.scrollIntoViewIfNeeded();
    await keyDetails.click();
    assert.equal(await page.getByRole('region', { name: '数字后使用英文标点说明', exact: true }).locator('.key-demo').count(), 1);
    await details.click();
    assert.equal(await details.getAttribute('aria-expanded'), 'false');
    assert.equal(await panel.isVisible(), false);
    await keyDetails.click();
    report.settingDetails = { closedByDefault: true, opensWithDemo: true, closesAgain: true };
  });
  await check('Next-word suggestion setting persists restart', async () => {
    await navigate(pages[1]);
    const original = (await state()).settings;
    assert.equal(original.predictionEnabled, false);
    // Enable, restart, then disable again: both states persist.
    await page.getByRole('switch', { name: '联想下一个词', exact: true }).click();
    await waitState(value => value.settings.predictionEnabled === true);
    await app.close(); await launch(); await navigate(pages[1]);
    assert.deepEqual((await state()).settings, { ...original, predictionEnabled: true });
    assert.equal(await page.evaluate(() => window.localPinyin.initialState.settings.predictionEnabled), true);
    assert.equal(await page.getByRole('switch', { name: '联想下一个词', exact: true }).getAttribute('aria-checked'), 'true');
    await page.getByRole('switch', { name: '联想下一个词', exact: true }).click();
    await waitState(value => value.settings.predictionEnabled === false);
    await app.close(); await launch(); await navigate(pages[1]);
    const disabled = await state();
    assert.deepEqual(disabled.settings, { ...original, predictionEnabled: false });
    assert.equal(await page.evaluate(() => window.localPinyin.initialState.settings.predictionEnabled), false);
    assert.equal(await page.getByRole('switch', { name: '联想下一个词', exact: true }).getAttribute('aria-checked'), 'false');
    assert.deepEqual((await state()).settings, original);
    report.predictionSetting = { disabledRestartVerified: true, enabledRestartVerified: true };
  });
  await check('Tab to English is off by default, saves for the IME and takes Tab from suggestions', async () => {
    await navigate(pages[1]);
    const original = (await state()).settings;
    assert.equal(original.tabTranslate, false);
    const translate = page.getByRole('switch', { name: 'Tab 键翻译成英文', exact: true });
    const prediction = page.getByRole('switch', { name: '联想下一个词', exact: true });
    assert.equal(await prediction.isDisabled(), false);
    await translate.scrollIntoViewIfNeeded();
    await translate.click();
    const on = await waitState(value => value.settings.tabTranslate === true);
    assert.deepEqual(on.settings, { ...original, tabTranslate: true }, 'Only the translation setting changed.');
    assert.equal(native('state').settings.tabTranslate, true, 'Saved for the IME.');
    // Tab now translates: the suggestion switch shows off and cannot be turned on.
    await page.waitForFunction(() => document.querySelector('[aria-label="联想下一个词"]')?.disabled === true);
    assert.equal(await prediction.getAttribute('aria-checked'), 'false');
    await screenshot('tab-translate');
    await translate.click();
    await waitState(value => value.settings.tabTranslate === false);
    await page.waitForFunction(() => document.querySelector('[aria-label="联想下一个词"]')?.disabled === false);
    assert.deepEqual((await state()).settings, original);
    report.tabTranslate = { offByDefault: true, saved: true, suggestionsYield: true };
  });
  await check('Personal words add/edit/search/delete and restart', async () => {
    await navigate(pages[2]);
    await page.locator('#page-dictionary .page-header').getByRole('button', { name: '添加词语', exact: true }).click();
    const add = page.getByRole('dialog', { name: '添加词语', exact: true });
    await add.getByLabel('中文词语', { exact: true }).fill('青石科技发展有限公司');
    await add.getByLabel('拼音', { exact: true }).fill('qingshikejifazhanyouxiangongsi');
    await add.getByLabel(/^词频/).fill('5000');
    await add.getByRole('button', { name: '保存词语', exact: true }).click();
    await page.getByRole('button', { name: '编辑 青石科技发展有限公司', exact: true }).waitFor();
    assert.equal(native('lookup', ['qingshikejifazhanyouxiangongsi']).candidates[0], '青石科技发展有限公司');
    await app.close(); await launch(); await navigate(pages[2]);
    await page.getByRole('button', { name: '编辑 青石科技发展有限公司', exact: true }).click();
    const edit = page.getByRole('dialog', { name: '编辑词语', exact: true });
    await edit.getByLabel('中文词语', { exact: true }).fill('青石科技服务有限公司');
    await edit.getByLabel('拼音', { exact: true }).fill('qingshikejifuwuyouxiangongsi');
    await edit.getByRole('button', { name: '保存词语', exact: true }).click();
    await page.getByRole('button', { name: '编辑 青石科技服务有限公司', exact: true }).waitFor();
    const entries = (await api('listUserLexicon')).entries;
    assert.deepEqual(entries, [{ pinyin: 'qingshikejifuwuyouxiangongsi', word: '青石科技服务有限公司', frequency: 5000 }]);
    assert.equal(native('lookup', [entries[0].pinyin]).candidates[0], entries[0].word);
    const search = page.getByLabel('搜索个人词库', { exact: true });
    await search.fill('qingshikeji');
    await page.getByRole('button', { name: '编辑 青石科技服务有限公司', exact: true }).waitFor();
    await navigate(pages[3]); await navigate(pages[2]);
    assert.equal(await search.inputValue(), 'qingshikeji', 'Sidebar navigation cleared the personal-word search.');
    await search.fill('无匹配测试词语甲乙丙丁');
    assert.equal(await page.getByRole('button', { name: '编辑 青石科技服务有限公司', exact: true }).count(), 0);
    await screenshot('personal-search-empty');
    await search.fill('');
    await screenshot('personal-word-added');
  });
  await check('Builtin dictionary uses native results', async () => {
    const result = await api('searchDictionary', ['随便', 40]);
    assert(result.total > 0 && result.entries.some(entry => entry.word === '随便' && entry.pinyin === 'suibian'));
    assert.deepEqual((await api('searchDictionary', ['无匹配测试词语甲乙丙丁', 40])).entries, []);
    await page.getByRole('button', { name: '浏览内置词库', exact: true }).click();
    const dialog = page.getByRole('dialog', { name: '内置词库', exact: true });
    await dialog.getByLabel('搜索内置词库', { exact: true }).fill('随便');
    await dialog.getByRole('cell', { name: '随便', exact: true }).waitFor();
    await screenshot('builtin-search-suibian');
    await dialog.getByLabel('搜索内置词库', { exact: true }).fill('无匹配测试词语甲乙丙丁');
    await dialog.getByRole('cell', { name: '随便', exact: true }).waitFor({ state: 'hidden' });
    await screenshot('builtin-search-empty');
    await page.keyboard.press('Escape');
    await dialog.waitFor({ state: 'hidden' });
  });
  await check('The sidebar is a candidate list in the chosen style; the palette commits with digits', async () => {
    await navigate(pages[0]);
    const nav = page.locator('nav');
    // Numbered like candidates (Ctrl+1–7); the numbers stay out of the names.
    assert.deepEqual(await nav.locator('.nav-item').evaluateAll(items => items.map(item => item.dataset.number)), ['1', '2', '3', '4', '5', '6', '7']);
    assert.equal(await nav.getByRole('button', { name: '输入', exact: true }).count(), 1);
    for (const style of ['capsule', 'marker', 'flow']) {
      await chooseStyle(style);
      await page.waitForFunction(value => document.querySelector('nav')?.dataset.candStyle === value, style);
      if (style === 'marker') await screenshot('navigation-marker');
    }
    // Ctrl+K, pinyin, then a digit runs that result, as in the candidate window.
    await page.keyboard.press('Control+k');
    const palette = page.getByRole('dialog', { name: '命令面板', exact: true });
    await palette.waitFor();
    await page.keyboard.type('ciku');
    await palette.locator('.palette-item').first().waitFor();
    assert.match(await palette.locator('.palette-item').first().textContent(), /词库/);
    await screenshot('palette-digits');
    await page.keyboard.press('1');
    await palette.waitFor({ state: 'detached' });
    await page.getByRole('heading', { name: '词库', exact: true }).waitFor();
    assert.equal(await page.locator('#page-dictionary').isVisible(), true);
  });
  await check('Keyboard navigation and dialog focus', async () => {
    const appearance = page.locator('nav').getByRole('button', { name: '外观', exact: true });
    await appearance.focus();
    await page.keyboard.press('Tab');
    assert.equal(await page.evaluate(() => document.activeElement?.textContent.trim()), '输入');
    await page.keyboard.press('Enter');
    await page.getByRole('heading', { name: '输入', exact: true }).waitFor();
    assert.equal(await page.evaluate(() => document.activeElement?.matches(':focus-visible')), true);
    await navigate(pages[2]);
    const addButton = page.locator('#page-dictionary .page-header').getByRole('button', { name: '添加词语', exact: true });
    await addButton.focus();
    await page.keyboard.press('Enter');
    const dialog = page.getByRole('dialog', { name: '添加词语', exact: true });
    await dialog.waitFor();
    for (let index = 0; index < 12; index++) {
      await page.keyboard.press('Tab');
      assert.equal(await page.evaluate(() => !!document.activeElement?.closest('[role="dialog"]')), true, 'Tab escaped the modal.');
    }
    await page.keyboard.press('Escape');
    await dialog.waitFor({ state: 'hidden' });
    assert.equal(await addButton.evaluate(element => document.activeElement === element), true);
  });
  await check('Clear-learning confirmation preserves personal words and settings', async () => {
    const before = await state();
    await app.close();
    const learning = path.join(before.dataDirectory, 'user_learning.tsv');
    const lexicon = path.join(before.dataDirectory, 'user_lexicon.tsv');
    const settings = path.join(before.dataDirectory, 'settings.json');
    const learned = `nihao\t你好\t3\t${Math.floor(Date.now() / 1000)}\nsuibian\t随便\t1\t${Math.floor(Date.now() / 1000)}\n`;
    fs.writeFileSync(learning, learned, 'utf8');
    const userBytes = fs.readFileSync(lexicon);
    const settingsBytes = fs.readFileSync(settings);
    await launch();
    assert.equal((await state()).learning.count, 2);
    await navigate(pages[3]);
    await page.getByRole('button', { name: '清除学习记录', exact: true }).click();
    const confirmation = page.getByRole('dialog', { name: '清除学习记录？', exact: true });
    await screenshot('learning-confirm');
    await confirmation.getByRole('button', { name: '取消', exact: true }).click();
    assert.equal(fs.readFileSync(learning, 'utf8'), learned);
    // 学到的词: searched, and one choice forgotten.
    const words = page.locator('section[aria-labelledby="learned-title"]');
    const rows = () => page.locator('section[aria-labelledby="learned-title"] tbody tr').count();
    await words.locator('tbody tr').nth(1).waitFor();
    assert.equal(await rows(), 2);
    await words.getByLabel('搜索学到的词').fill('sui');
    await page.waitForFunction(() => document.querySelectorAll('section[aria-labelledby="learned-title"] tbody tr').length === 1);
    await words.getByLabel('搜索学到的词').fill('');
    await words.locator('tbody tr').nth(1).waitFor();
    await words.getByRole('button', { name: '删除 nihao 你好', exact: true }).click();
    await page.waitForFunction(() => document.querySelectorAll('section[aria-labelledby="learned-title"] tbody tr').length === 1);
    assert.equal((await state()).learning.count, 1);
    assert.ok(!fs.readFileSync(learning, 'utf8').includes('你好'));
    await screenshot('learning-words');
    await page.getByRole('button', { name: '清除学习记录', exact: true }).click();
    await confirmation.getByRole('button', { name: '清除记录', exact: true }).click();
    await confirmation.waitFor({ state: 'hidden' });
    assert.equal((await state()).learning.count, 0);
    assert.equal(fs.existsSync(learning), false);
    assert.deepEqual(fs.readFileSync(lexicon), userBytes);
    assert.deepEqual(fs.readFileSync(settings), settingsBytes);
    assert.equal((await api('listUserLexicon')).entries[0].word, '青石科技服务有限公司');
    await screenshot('learning-cleared');
  });
  await check('Personal-word deletion confirmation persists restart', async () => {
    await navigate(pages[2]);
    const remove = page.getByRole('button', { name: '删除 青石科技服务有限公司', exact: true });
    await remove.click();
    const dialog = page.getByRole('dialog', { name: '删除这个词语？', exact: true });
    await dialog.getByRole('button', { name: '取消', exact: true }).click();
    assert.equal((await api('listUserLexicon')).entries.length, 1);
    await remove.click();
    await dialog.getByRole('button', { name: '删除词语', exact: true }).click();
    await dialog.waitFor({ state: 'hidden' });
    assert.deepEqual((await api('listUserLexicon')).entries, []);
    await app.close(); await launch();
    assert.deepEqual((await api('listUserLexicon')).entries, []);
    await navigate(pages[2]);
    await screenshot('personal-deleted');
  });
  await check('Invalid bridge payloads cannot alter saved data', async () => {
    const before = await state();
    const filesBefore = savedFiles(before.dataDirectory);
    for (const request of [
      ['saveSettings', [{ ...before.settings, theme: 'arbitrary' }]],
      ['saveSettings', [{ ...before.settings, textSize: -100 }]],
      ['saveSettings', [{ ...before.settings, predictionEnabled: 'true' }]],
      ['saveSettings', [{ ...before.settings, predictionEnabled: undefined }]],
      ['saveSettings', [{ ...before.settings, candidateStyle: 'aurora' }]],
      ['saveSettings', [{ ...before.settings, candidateStyle: undefined }]],
      ['saveSettings', [{ ...before.settings, fuzzyPinyin: ['zz'] }]],
      ['saveSettings', [{ ...before.settings, fuzzyPinyin: 'z-zh' }]],
      ['saveSettings', [{ ...before.settings, doublePinyin: 'qwerty' }]],
      ['saveSettings', [{ ...before.settings, typoCorrection: 'yes' }]],
      ['upsertUserEntry', [{ pinyin: '../outside', word: '无效词', frequency: 5000 }]],
      ['lookup', ['qu汉', '我想']],
      ['lookup', ['q'.repeat(257), '我想']],
      ['lookup', ['qu', '中'.repeat(33)]],
      ['lookup', [{ command: 'state' }, '我想']],
      ['predict', ['中'.repeat(33)]],
      ['predict', ['我想\n去']],
      ['predict', [{ command: 'state' }]],
      ['windowControl', ['run-command']],
    ]) {
      const rejected = await page.evaluate(async ({ name, args }) => { try { await window.localPinyin[name](...args); return false; } catch { return true; } }, { name: request[0], args: request[1] });
      assert.equal(rejected, true, `Invalid ${request[0]} payload was accepted.`);
    }
    assert.deepEqual((await state()).settings, before.settings);
    assert.equal((await api('listUserLexicon')).entries.length, 0);
    assert.deepEqual(savedFiles(before.dataDirectory), filesBefore, 'Rejected IPC calls altered personal files.');
  });
  await check('Updates: a newer release is found, downloaded, verified and handed to its installer', async () => {
    await navigate(pages[4]);
    const panelText = () => page.locator('.update-panel').textContent();
    const waitPanel = pattern => page.waitForFunction(source =>
      new RegExp(source).test(document.querySelector('.update-panel')?.textContent ?? ''), pattern.source);
    // The check when settings opened found nothing newer.
    await waitPanel(/已是最新版本/);
    assert.equal(await page.locator('.nav-badge').count(), 0);
    // A newer release: found, shown with its notes, flagged in the sidebar.
    feed.installer = crypto.randomBytes(3 * 1024 * 1024);
    feed.checksum = sha256(feed.installer);
    feed.version = '99.0.0';
    await page.getByRole('button', { name: '检查更新', exact: true }).click();
    await waitPanel(/发现新版本 99\.0\.0/);
    assert.match(await panelText(), /自动检测更新/);
    assert.match(await panelText(), /3\.0 MB/);
    assert.equal(await page.locator('.nav-badge').textContent(), '新版本');
    await screenshot('update-available');
    // Downloaded, checked against its SHA-256 file and kept for the installer.
    await page.getByRole('button', { name: '下载新版本', exact: true }).click();
    await waitPanel(/已下载并通过 SHA-256 校验/);
    const updates = path.join(data, 'LocalPinyinIME', 'updates');
    const downloaded = path.join(updates, 'LocalPinyinIME-99.0.0-x64.exe');
    assert.equal(sha256(fs.readFileSync(downloaded)), feed.checksum);
    assert(!fs.existsSync(downloaded + '.partial'));
    await screenshot('update-ready');
    await page.getByRole('button', { name: '立即安装', exact: true }).click();
    await page.getByText('安装程序已启动，按提示完成安装', { exact: true }).waitFor();
    // A tampered installer is refused and removed.
    feed.version = '99.0.1';
    feed.installer = crypto.randomBytes(1024 * 1024);
    feed.checksum = sha256(Buffer.from('not the installer'));
    // (A newer version on screen offers only the next step; check through the API.)
    await page.evaluate(() => window.localPinyin.checkForUpdate());
    await waitPanel(/发现新版本 99\.0\.1/);
    await page.getByRole('button', { name: '下载新版本', exact: true }).click();
    await waitPanel(/校验失败/);
    assert(!fs.existsSync(path.join(updates, 'LocalPinyinIME-99.0.1-x64.exe')));
    assert(!fs.existsSync(path.join(updates, 'LocalPinyinIME-99.0.1-x64.exe.partial')));
    // Automatic checks can be turned off, and the choice is kept.
    const auto = page.getByRole('switch', { name: '自动检查更新', exact: true });
    assert.equal(await auto.getAttribute('aria-checked'), 'true');
    await auto.click();
    await page.waitForFunction(() => document.querySelector('[aria-label="自动检查更新"]')?.getAttribute('aria-checked') === 'false');
    const preferences = JSON.parse(fs.readFileSync(path.join(data, 'LocalPinyinIME', 'desktop', 'update.json'), 'utf8'));
    assert.equal(preferences.autoCheck, false);
    await auto.click();
    await page.waitForFunction(() => document.querySelector('[aria-label="自动检查更新"]')?.getAttribute('aria-checked') === 'true');
    // Back to no newer release for the checks that follow.
    feed.version = null;
    await page.getByRole('button', { name: '重试', exact: true }).click();
    await waitPanel(/已是最新版本/);
    assert.equal(await page.locator('.nav-badge').count(), 0);
    assert(feed.requests.every(url => url.startsWith('/releases') || url.startsWith('/download/')), 'Only the update feed was asked.');
    report.updates = { found: '99.0.0', verified: true, tamperedRefused: true, autoCheckToggle: true, requests: feed.requests.length };
  });
  await check('本地英文 has its own page: on or off, switch key, typing options and keys', async () => {
    await navigate(pages.find(item => item.id === 'english'));
    const english = page.locator('#page-english');
    const ime = english.getByRole('switch', { name: '本地英文输入法', exact: true });
    assert.equal(await ime.getAttribute('aria-checked'), 'true');
    assert.match(await english.locator('.english-hero').textContent(), /Alt\s*\+?\s*CapsLock/);
    const keys = english.getByRole('radiogroup', { name: '切换输入法的快捷键' });
    await keys.locator('label', { hasText: 'Ctrl + CapsLock' }).click();
    await waitState(value => value.settings.imeSwitchKey === 'ctrl+capslock');
    const sentence = english.getByRole('switch', { name: '整句输入', exact: true });
    await sentence.click();
    await waitState(value => value.settings.englishSentence === false);
    // Space twice is only listed for sentences.
    await english.getByText('放进改好的整句', { exact: true }).waitFor({ state: 'detached' });
    await sentence.click();
    await waitState(value => value.settings.englishSentence === true);
    await english.getByText('放进改好的整句', { exact: true }).waitFor();
    // Off: the data folder here is a test one, so only the setting changes;
    // the switch key is then neither taken nor chosen.
    await ime.click();
    await waitState(value => value.settings.englishIme === false);
    assert.match(JSON.stringify(native('state').settings), /"englishIme":false/);
    await english.getByText('切换到本地拼音', { exact: true }).waitFor({ state: 'detached' });
    assert.equal(await keys.locator('input').first().isDisabled(), true);
    await screenshot('english-off');
    await ime.click();
    await waitState(value => value.settings.englishIme === true);
    await keys.locator('label', { hasText: 'Alt + CapsLock' }).click();
    await waitState(value => value.settings.imeSwitchKey === 'alt+capslock');
    await english.getByText('切换到本地拼音', { exact: true }).waitFor();
    await screenshot('english-page');
    // The input page no longer repeats these settings.
    assert.equal(await page.locator('#page-input').getByRole('switch', { name: '整句输入', exact: true }).count(), 0);
  });
  await check('工具面板 settings save for the IME', async () => {
    await navigate(pages.find(item => item.id === 'panel'));
    const doubleTab = page.getByRole('switch', { name: '连按两下 Tab 打开工具面板', exact: true });
    assert.equal(await doubleTab.getAttribute('aria-checked'), 'true');
    await doubleTab.click();
    await waitState(value => value.settings.panelDoubleTab === false);
    await doubleTab.click();
    await waitState(value => value.settings.panelDoubleTab === true);
    await page.getByRole('switch', { name: '大窗口在后台待命', exact: true }).click();
    await waitState(value => value.settings.panelResident === true);
    await page.getByRole('switch', { name: '大窗口在后台待命', exact: true }).click();
    await waitState(value => value.settings.panelResident === false);
    await page.getByRole('radiogroup', { name: '剪贴板保存时间' }).locator('label', { hasText: '30 天' }).click();
    await waitState(value => value.settings.clipboardRetentionDays === 30);
    await page.getByRole('radiogroup', { name: '剪贴板保存时间' }).locator('label', { hasText: '永久' }).click();
    await waitState(value => value.settings.clipboardRetentionDays === 0);
    assert.equal(native('state').settings.clipboardRetentionDays, 0);
    await page.getByRole('switch', { name: '用过的表情出现在候选', exact: true }).click();
    await waitState(value => value.settings.emojiCandidates === false);
    await page.getByRole('switch', { name: '用过的表情出现在候选', exact: true }).click();
    await waitState(value => value.settings.emojiCandidates === true);
    await screenshot('panel-settings');
  });
  await check('输入习惯 are added and deleted in settings and drive the IME', async () => {
    await navigate(pages.find(item => item.id === 'learning'));
    await page.getByLabel('内容', { exact: true }).fill('$');
    await page.getByLabel('短语', { exact: true }).fill('美元，美刀');
    await page.locator('.habit-form').getByRole('button', { name: '添加', exact: true }).click();
    await page.locator('.habits .lexicon-table tbody tr').nth(1).waitFor();
    assert.equal(native('lookup', ['meiyuan']).candidates[0], '$');
    assert.equal(native('lookup', ['meidao']).candidates[0], '$');
    await screenshot('habits');
    await page.getByRole('button', { name: '删除短语 美刀', exact: true }).click();
    await page.waitForFunction(() => document.querySelectorAll('.habits .lexicon-table tbody tr').length === 1);
    assert.notEqual(native('lookup', ['meidao']).candidates[0], '$');
  });
  await check('记录 passwords are set, changed, removed and the records deleted in settings', async () => {
    await navigate(pages.find(item => item.id === 'panel'));
    const group = page.locator('section[aria-labelledby="group-records"]');
    await group.getByRole('button', { name: '设置密码', exact: true }).click();
    let dialog = page.getByRole('dialog', { name: '设置记录密码', exact: true });
    await dialog.getByLabel('新密码', { exact: true }).fill('一二三');
    await dialog.getByLabel('再输入一次', { exact: true }).fill('一二三');
    await dialog.getByRole('button', { name: '确定', exact: true }).click();
    await page.locator('.toast', { hasText: '已设置记录密码' }).waitFor();
    assert.deepEqual(native('vault', ['status']), { exists: true, password: true });
    await group.getByRole('button', { name: '修改密码', exact: true }).click();
    dialog = page.getByRole('dialog', { name: '修改记录密码', exact: true });
    await dialog.getByLabel('现在的密码', { exact: true }).fill('不对');
    await dialog.getByLabel('新密码', { exact: true }).fill('四五六');
    await dialog.getByLabel('再输入一次', { exact: true }).fill('四五六');
    await dialog.getByRole('button', { name: '确定', exact: true }).click();
    await dialog.locator('.error-notice', { hasText: '原来的密码不对' }).waitFor();
    await dialog.getByLabel('现在的密码', { exact: true }).fill('一二三');
    await dialog.getByRole('button', { name: '确定', exact: true }).click();
    await page.locator('.toast', { hasText: '已修改记录密码' }).waitFor();
    await screenshot('records-settings');
    await group.getByRole('button', { name: '取消密码', exact: true }).click();
    dialog = page.getByRole('dialog', { name: '取消记录密码', exact: true });
    await dialog.getByLabel('现在的密码', { exact: true }).fill('四五六');
    await dialog.getByRole('button', { name: '确定', exact: true }).click();
    await page.locator('.toast', { hasText: '已取消记录密码' }).waitFor();
    assert.deepEqual(native('vault', ['status']), { exists: true, password: false });
    await group.getByRole('button', { name: '删除全部记录', exact: true }).click();
    await page.getByRole('dialog', { name: '删除全部记录？', exact: true }).getByRole('button', { name: '删除全部记录', exact: true }).click();
    await page.locator('.toast', { hasText: '已删除全部记录' }).waitFor();
    assert.deepEqual(native('vault', ['status']), { exists: false, password: false });
  });
  await check('User data is exported, then imported part by part', async () => {
    await navigate(pages.find(item => item.id === 'about'));
    await page.getByRole('button', { name: '快速导出', exact: true }).click();
    await page.locator('.toast', { hasText: '已导出' }).waitFor();
    assert(fs.statSync(process.env.LOCALPINYINIME_QA_BACKUP_FILE).size > 100);
    native('phrases-delete', ['meiyuan', '$']);
    assert.notEqual(native('lookup', ['meiyuan']).candidates[0], '$');
    await page.getByRole('button', { name: '从备份导入…', exact: true }).click();
    const dialog = page.getByRole('dialog', { name: '导入哪些内容？', exact: true });
    await dialog.waitFor();
    await dialog.getByRole('radiogroup', { name: '设置的导入方式', exact: true }).locator('label', { hasText: '跳过' }).click();
    await screenshot('import-choices');
    await dialog.getByRole('button', { name: /^导入 \d+ 项$/ }).click();
    await page.locator('.toast', { hasText: '已导入' }).or(dialog.locator('.error-notice')).first().waitFor();
    assert.equal(await dialog.locator('.error-notice').count(), 0, await dialog.locator('.error-notice').allInnerTexts().then(texts => texts.join(' ')));
    assert.equal(native('lookup', ['meiyuan']).candidates[0], '$');
  });
  await check('External navigation and windows are denied', async () => {
    const url = page.url();
    const count = app.windows().length;
    await page.evaluate(() => { const a = document.createElement('a'); a.href = 'https://example.com/qa-denied'; a.textContent = 'external'; document.body.append(a); a.click(); a.remove(); window.open('https://example.com/qa-popup'); });
    await page.waitForTimeout(150);
    assert.equal(page.url(), url);
    assert.equal(app.windows().length, count);
    assert.equal(new URL(url).protocol, 'app:');
  });
  assert.deepEqual(report.errors, [], 'Renderer errors occurred.');
  await app.close(); app = undefined;
  report.completedAt = new Date().toISOString();
  report.passed = true;
}

run().catch(async error => {
  report.passed = false;
  report.failure = error.stack || String(error);
  process.stderr.write(`${report.failure}\n`);
  try { if (page && !page.isClosed()) await screenshot('failure'); } catch {}
  try { if (app) await app.close(); } catch {}
  process.exitCode = 1;
}).finally(() => {
  feedServer.close();
  fs.writeFileSync(path.join(output, 'verification.json'), JSON.stringify(report, null, 2) + '\n');
});
