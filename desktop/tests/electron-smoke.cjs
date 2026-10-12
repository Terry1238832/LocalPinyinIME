#!/usr/bin/env node
/* Real Electron + native bridge integration; no renderer or IPC mocks. */
'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const zlib = require('node:zlib');

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

async function launch() {
  const environment = { ...process.env };
  delete environment.ELECTRON_RUN_AS_NODE;
  delete environment.LOCALPINYINIME_USER_LEXICON_PATH;
  delete environment.LOCALPINYINIME_DIAGNOSTICS;
  app = await _electron.launch({
    executablePath: executable,
    args: cli.packaged ? [] : [desktop],
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
async function navigate(id) {
  await page.locator('nav').getByRole('button', { name: id.label, exact: true }).click();
  await page.getByRole('heading', { name: id.heading, exact: true }).waitFor();
  assert.equal(await page.locator(`#page-${id.id}`).isVisible(), true);
}
const pages = [
  { id: 'appearance', label: '外观', heading: '外观' },
  { id: 'input', label: '输入', heading: '输入习惯' },
  { id: 'dictionary', label: '词库', heading: '我的词库' },
  { id: 'learning', label: '学习', heading: '个性学习' },
  { id: 'about', label: '关于', heading: '关于 Local Pinyin' },
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
  await page.waitForFunction(size => innerWidth === size.width && innerHeight === size.height, { width, height });
}

async function noOverflow() {
  const measurements = await page.evaluate(() => ({ width: innerWidth, scrollWidth: document.documentElement.scrollWidth, body: document.body.scrollWidth }));
  assert(measurements.scrollWidth <= measurements.width + 1 && measurements.body <= measurements.width + 1, `Horizontal overflow: ${JSON.stringify(measurements)}`);
}

async function api(name, args = []) {
  return page.evaluate(({ name, args }) => window.localPinyin[name](...args), { name, args });
}

async function run() {
  await launch();
  await check('Real native state and preview', async () => {
    const initial = await state();
    assert.equal(initial.version, expectedVersion);
    assert(initial.dictionary.total >= 83062, 'Built-in resources were not loaded.');
    assert.equal(path.resolve(initial.dataDirectory), path.join(data, 'LocalPinyinIME'));
    assert.deepEqual((await api('listUserLexicon')).entries, []);
    assert.equal((await api('lookup', ['suibian'])).candidates[0], '随便');
    assert.deepEqual((await api('lookup', [''])).candidates, []);
    const preview = page.getByLabel('试试拼音', { exact: true });
    await preview.fill('suibian');
    await page.getByLabel('候选预览', { exact: true }).getByText('随便', { exact: true }).waitFor();
    await preview.fill('zzzzzzzzzzzzzz');
    await page.waitForFunction(() => !document.querySelector('[aria-label="候选预览"]')?.textContent.includes('随便'));
    await screenshot('appearance-unknown');
    await preview.fill('');
    await screenshot('appearance-empty');
    await preview.fill('suibian');
    await page.getByLabel('候选预览', { exact: true }).getByText('随便', { exact: true }).waitFor();
  });
  await check('Electron renderer isolation and fixed bridge', async () => {
    const exposed = await page.evaluate(() => ({ require: typeof window.require, process: typeof window.process, api: Object.keys(window.localPinyin).sort(), frozen: Object.isFrozen(window.localPinyin) }));
    assert.equal(exposed.require, 'undefined');
    assert.equal(exposed.process, 'undefined');
    assert.equal(exposed.frozen, true);
    assert.deepEqual(exposed.api, ['clearLearning', 'deleteUserEntry', 'getState', 'initialState', 'listUserLexicon', 'lookup', 'openDataDirectory', 'saveSettings', 'searchDictionary', 'upsertUserEntry', 'windowControl'].sort());
    assert.equal(await page.evaluate(() => typeof window.localPinyin.initialState), 'object');
    assert.equal(await page.evaluate(() => window.localPinyin.initialState.version), expectedVersion);
    const preferences = await app.evaluate(({ BrowserWindow }) => BrowserWindow.getAllWindows()[0].webContents.getLastWebPreferences());
    assert.equal(preferences.nodeIntegration, false);
    assert.equal(preferences.contextIsolation, true);
    assert.equal(preferences.sandbox, true);
    assert.equal(preferences.webSecurity, true);
  });
  await check('Rapid side navigation preserves shell and all page state', async () => {
    const beforeNavigation = navigationCount;
    const observations = await page.evaluate(async () => {
      const shell = document.querySelector('#root')?.firstElementChild;
      const nav = document.querySelector('nav');
      const sections = Array.from(document.querySelectorAll('section[id^="page-"]'));
      const expected = ['appearance', 'input', 'dictionary', 'learning', 'about'];
      const buttons = Array.from(nav.querySelectorAll('button'));
      const failures = [];
      let frames = 0;
      const nextFrame = () => new Promise((resolve, reject) => {
        const timeout = setTimeout(() => reject(new Error(`Hidden-window RAF did not paint (visibility=${document.visibilityState}); enable QA offscreen rendering.`)), 1500);
        requestAnimationFrame(() => { clearTimeout(timeout); resolve(); });
      });
      for (let iteration = 0; iteration < 80; iteration++) {
        const index = iteration % 5;
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
      return { failures, frames, sections: sections.length, samePreview: document.querySelector('#preview-pinyin')?.value === 'suibian' };
    });
    assert.deepEqual(observations.failures, []);
    assert.equal(observations.sections, 5);
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
    const expectedShift = !(await state()).settings.shiftToggleEnabled;
    const shift = page.getByRole('switch');
    await shift.click();
    await waitState(value => value.settings.shiftToggleEnabled === expectedShift);
    const saved = (await state()).settings;
    await app.close();
    await launch();
    assert.deepEqual((await state()).settings, saved);
    assert.deepEqual(await page.evaluate(() => window.localPinyin.initialState.settings), saved);
    await navigate(pages[1]);
    assert.equal(await page.getByRole('switch').getAttribute('aria-checked'), String(saved.shiftToggleEnabled));
    report.persistedSettings = saved;
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
    assert.equal((await api('lookup', ['qingshikejifazhanyouxiangongsi'])).candidates[0], '青石科技发展有限公司');
    await app.close(); await launch(); await navigate(pages[2]);
    await page.getByRole('button', { name: '编辑 青石科技发展有限公司', exact: true }).click();
    const edit = page.getByRole('dialog', { name: '编辑词语', exact: true });
    await edit.getByLabel('中文词语', { exact: true }).fill('青石科技服务有限公司');
    await edit.getByLabel('拼音', { exact: true }).fill('qingshikejifuwuyouxiangongsi');
    await edit.getByRole('button', { name: '保存词语', exact: true }).click();
    await page.getByRole('button', { name: '编辑 青石科技服务有限公司', exact: true }).waitFor();
    const entries = (await api('listUserLexicon')).entries;
    assert.deepEqual(entries, [{ pinyin: 'qingshikejifuwuyouxiangongsi', word: '青石科技服务有限公司', frequency: 5000 }]);
    assert.equal((await api('lookup', [entries[0].pinyin])).candidates[0], entries[0].word);
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
  await check('Keyboard navigation and dialog focus', async () => {
    const appearance = page.locator('nav').getByRole('button', { name: '外观', exact: true });
    await appearance.focus();
    await page.keyboard.press('Tab');
    assert.equal(await page.evaluate(() => document.activeElement?.textContent.trim()), '输入');
    await page.keyboard.press('Enter');
    await page.getByRole('heading', { name: '输入习惯', exact: true }).waitFor();
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
    for (const request of [
      ['saveSettings', [{ ...before.settings, theme: 'arbitrary' }]],
      ['saveSettings', [{ ...before.settings, textSize: -100 }]],
      ['upsertUserEntry', [{ pinyin: '../outside', word: '无效词', frequency: 5000 }]],
      ['windowControl', ['run-command']],
    ]) {
      const rejected = await page.evaluate(async ({ name, args }) => { try { await window.localPinyin[name](...args); return false; } catch { return true; } }, { name: request[0], args: request[1] });
      assert.equal(rejected, true, `Invalid ${request[0]} payload was accepted.`);
    }
    assert.deepEqual((await state()).settings, before.settings);
    assert.equal((await api('listUserLexicon')).entries.length, 0);
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
  fs.writeFileSync(path.join(output, 'verification.json'), JSON.stringify(report, null, 2) + '\n');
});
