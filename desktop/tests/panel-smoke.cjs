#!/usr/bin/env node
/* The tool panel in real Electron with the native bridge, no mocks: pages,
   the tray, 添加短语, tools, the clipboard history and stickers. Inserting
   into another application is covered by the native tests. */
'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const zlib = require('node:zlib');
const { spawnSync } = require('node:child_process');

const cli = Object.fromEntries(process.argv.slice(2).reduce((items, value, index, args) => {
  if (value.startsWith('--')) items.push([value.slice(2), args[index + 1]?.startsWith('--') ? true : args[index + 1] ?? true]);
  return items;
}, []));
const desktop = path.resolve(cli.app || path.join(__dirname, '..'));
const source = path.resolve(__dirname, '../..');
const output = path.resolve(cli.output || path.join(source, '.build/panel-ui'));
const data = path.resolve(cli.data || path.join(output, `isolated-data-${Date.now()}`));
const bridge = path.resolve(cli.bridge || path.join(source, '.build/bin/LocalPinyinSettingsBridge.exe'));
const executable = path.resolve(cli.executable || path.join(desktop, 'node_modules/electron/dist/electron.exe'));
const { _electron } = require(process.env.LOCALPINYINIME_PLAYWRIGHT_MODULE || 'playwright');

fs.mkdirSync(output, { recursive: true });
assert(!fs.existsSync(data) || fs.readdirSync(data).length === 0, 'Choose a fresh --data directory; this suite never replaces existing data.');
fs.mkdirSync(data, { recursive: true });
assert(fs.existsSync(bridge), `Missing native bridge: ${bridge}`);

const environment = { ...process.env, LOCALPINYINIME_LOCALAPPDATA: data, LOCALPINYINIME_NO_AUTOSTART: '1' };
delete environment.ELECTRON_RUN_AS_NODE;
delete environment.LOCALPINYINIME_USER_LEXICON_PATH;
delete environment.LOCALPINYINIME_DIAGNOSTICS;
const report = { startedAt: new Date().toISOString(), checks: [], screenshots: [], errors: [] };
let app;
let page;

function callBridge(...args) {
  const result = spawnSync(bridge, args, { env: environment, encoding: 'utf8' });
  return JSON.parse(result.stdout);
}

// A small solid PNG, for stickers and copied pictures.
function png(width, height, [r, g, b]) {
  const crcTable = Array.from({ length: 256 }, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
  const crc = buffer => { let c = 0xffffffff; for (const byte of buffer) c = crcTable[(c ^ byte) & 0xff] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; };
  const chunk = (type, body) => {
    const length = Buffer.alloc(4); length.writeUInt32BE(body.length);
    const typed = Buffer.concat([Buffer.from(type), body]);
    const sum = Buffer.alloc(4); sum.writeUInt32BE(crc(typed));
    return Buffer.concat([length, typed, sum]);
  };
  const header = Buffer.alloc(13);
  header.writeUInt32BE(width, 0); header.writeUInt32BE(height, 4); header[8] = 8; header[9] = 2;
  const rows = Buffer.alloc((width * 3 + 1) * height);
  for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) rows.set([r, g, b], y * (width * 3 + 1) + 1 + x * 3);
  return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk('IHDR', header), chunk('IDAT', zlib.deflateSync(rows)), chunk('IEND', Buffer.alloc(0))]);
}

async function check(name, fn) {
  const start = performance.now();
  await fn();
  report.checks.push({ name, milliseconds: Math.round(performance.now() - start), passed: true });
  process.stdout.write(`PASS ${name}\n`);
}
async function shot(name) {
  const file = path.join(output, name);
  await page.screenshot({ path: file });
  report.screenshots.push(file);
}
const tray = () => page.locator('.tray .tray-item').allInnerTexts();

async function main() {
  // Copies already in the history: a text and a picture with words read from it.
  const clipboard = path.join(data, 'LocalPinyinIME', 'clipboard');
  fs.mkdirSync(clipboard, { recursive: true });
  const now = Date.now();
  fs.writeFileSync(path.join(clipboard, 'aaaaaaaaaaaaaaaa.png'), png(320, 180, [70, 130, 220]));
  fs.writeFileSync(path.join(clipboard, 'history.jsonl'), [
    { id: 'bbbbbbbbbbbbbbbb', kind: 'text', time: now - 60_000, text: '明天上午十点在三楼会议室开会，记得带电脑。', hash: 'x' },
    { id: 'aaaaaaaaaaaaaaaa', kind: 'image', time: now - 3_600_000, w: 320, h: 180, ocr: '会议纪要\n第一项 预算', ocrDone: true, hash: 'y' },
  ].map(item => JSON.stringify(item)).join('\n') + '\n');

  app = await _electron.launch({
    executablePath: executable,
    args: [...(cli.packaged ? [] : [desktop]), '--panel', '--ime=1', '--host=2', `--token=${'a1'.repeat(16)}`, '--anchor=300,240,262',
      '--page=symbol'],
    cwd: desktop,
    env: { ...environment, LOCALPINYINIME_BRIDGE_PATH: bridge, LOCALPINYINIME_QA_HEADLESS: '1' },
    timeout: 30000,
  });
  page = await app.firstWindow();
  page.setDefaultTimeout(10000);
  page.on('pageerror', error => report.errors.push(String(error)));
  await page.waitForSelector('.panel .rail');
  await page.setViewportSize({ width: 780, height: 480 });

  await check('opens on 符号 with 常用数学符号 before 常规标点', async () => {
    assert.deepEqual(await page.locator('.rail-item span').allInnerTexts(), ['工具', '剪贴板', '符号', '颜文字', '表情', '表情包', '记录', '设置']);
    await page.locator('.cell.symbol').first().waitFor();
    const categories = await page.locator('.category').allInnerTexts();
    assert(categories.indexOf('常用数学符号') >= 0 && categories.indexOf('常用数学符号') < categories.indexOf('常规标点'), categories.join(' '));
    await shot('panel-symbols-light.png');
  });

  await check('symbols collect in the tray and come out again', async () => {
    await page.locator('.cell.symbol', { hasText: '≈' }).first().click();
    await page.locator('.cell.symbol', { hasText: '+' }).first().click();
    assert.deepEqual(await tray(), ['≈', '+']);
    await page.locator('.tray-item', { hasText: '≈' }).click();
    assert.deepEqual(await tray(), ['+']);
    await page.getByRole('button', { name: '清空', exact: true }).click();
    assert.deepEqual(await tray(), []);
  });

  await check('category bar jumps to a category', async () => {
    await page.getByRole('tab', { name: '希腊字母' }).click();
    await page.waitForFunction(() => document.querySelector('.category.active')?.textContent === '希腊字母');
    await page.locator('[data-group="greek"] .cell').first().waitFor();
  });

  await check('search finds $ by 美元, and 添加短语 ties words to it', async () => {
    await page.getByLabel(/^搜索符号/).fill('美元');
    await page.locator('.cell.symbol', { hasText: '$' }).first().click();
    assert.deepEqual(await tray(), ['$']);
    await page.getByRole('button', { name: '添加短语', exact: true }).click();
    const dialog = page.getByRole('dialog', { name: '添加短语' });
    await dialog.waitFor();
    const field = dialog.getByLabel('短语', { exact: true });
    await field.fill('美元'); await field.press('Enter');
    await field.fill('美刀'); await field.press('Enter');
    assert.deepEqual(await dialog.locator('.chip').allInnerTexts(), ['美元', '美刀']);
    await shot('panel-add-phrase.png');
    await dialog.getByRole('button', { name: '保存', exact: true }).click();
    await page.locator('.toast', { hasText: '美刀' }).waitFor();
    assert.equal(callBridge('lookup', 'meiyuan').candidates[0], '$');
    assert.equal(callBridge('lookup', 'meidao').candidates[0], '$');
    await page.getByLabel('清空搜索').click();
  });

  await check('right-click adds to 收藏', async () => {
    await page.locator('.cell.symbol', { hasText: '√' }).first().click({ button: 'right' });
    await page.getByRole('tab', { name: /收藏/ }).waitFor();
    await page.locator('[data-group="favorite"] .cell', { hasText: '√' }).waitFor();
  });

  await check('颜文字 by mood, 表情 with search', async () => {
    await page.getByRole('button', { name: '颜文字', exact: true }).click();
    await page.getByRole('tab', { name: '开心' }).waitFor();
    assert((await page.locator('.cell.kaomoji').count()) >= 900);
    await shot('panel-kaomoji.png');
    await page.getByRole('button', { name: '表情', exact: true }).click();
    await page.locator('.cell.emoji').first().waitFor();
    await shot('panel-emoji.png');
    await page.getByLabel(/^搜索表情/).fill('猫');
    assert((await page.locator('.cell.emoji').count()) >= 3);
    await page.getByLabel('清空搜索').click();
  });

  await check('工具 converts as it is typed', async () => {
    await page.getByRole('button', { name: '工具', exact: true }).click();
    await shot('panel-tools.png');
    const field = page.getByLabel('要转换的内容');
    await field.fill('55+15');
    await page.locator('.tool-value', { hasText: '55+15=70' }).waitFor();
    assert.deepEqual((await page.locator('.tool-value').allInnerTexts()).slice(0, 2), ['70', '55+15=70']);
    await field.fill('123');
    await page.locator('.tool-value', { hasText: '壹佰贰拾叁' }).first().waitFor();
    await field.fill('12.19');
    await page.locator('.tool-value', { hasText: '十二月十九日' }).first().waitFor();
    await shot('panel-tools-result.png');
  });

  await check('剪贴板 asks first, then lists, searches picture words and enlarges', async () => {
    await page.getByRole('button', { name: '剪贴板', exact: true }).click();
    await page.getByRole('heading', { name: '要记住复制过的内容吗？' }).waitFor();
    await shot('panel-clipboard-ask.png');
    await page.getByRole('button', { name: '开启剪贴板记录' }).click();
    await page.locator('.clip').first().waitFor();
    assert.equal(await page.locator('.clip').count(), 2);
    assert.equal(callBridge('state').settings.clipboardHistory, 'on');
    await shot('panel-clipboard.png');
    await page.getByLabel(/^搜索复制过的文字/).fill('会议纪要');
    await page.locator('.clip-ocr mark', { hasText: '会议纪要' }).waitFor();
    assert.equal(await page.locator('.clip').count(), 1);
    await page.locator('.clip.image .clip-main').click();
    await page.getByRole('dialog', { name: '查看图片' }).waitFor();
    await shot('panel-clipboard-viewer.png');
    await page.keyboard.press('Escape');
    await page.getByRole('dialog', { name: '查看图片' }).waitFor({ state: 'detached' });
    callBridge('clipboard-agent', 'stop');
  });

  await check('表情包 are added from files and deleted', async () => {
    await page.getByRole('button', { name: '表情包', exact: true }).click();
    await page.locator('input[type=file]').setInputFiles([
      { name: '开心.png', mimeType: 'image/png', buffer: png(96, 96, [250, 200, 60]) },
      { name: '点赞.png', mimeType: 'image/png', buffer: png(96, 96, [80, 200, 120]) },
    ]);
    await page.locator('.sticker').nth(1).waitFor();
    assert.equal(await page.locator('.sticker').count(), 2);
    await shot('panel-stickers.png');
    await page.locator('.sticker').first().hover();
    await page.getByRole('button', { name: /^删除 / }).first().click();
    await page.locator('.sticker-confirm').getByRole('button', { name: '删除', exact: true }).click();
    await page.waitForFunction(() => document.querySelectorAll('.sticker').length === 1);
  });

  await check('记录: a password first, then items are added, renamed, shown and locked', async () => {
    await page.getByRole('button', { name: '记录', exact: true }).click();
    await page.getByRole('heading', { name: '第一次使用记录' }).waitFor();
    await shot('panel-records-setup.png');
    await page.getByLabel('设置密码', { exact: true }).fill('开门123');
    await page.getByLabel('再输入一次密码', { exact: true }).fill('开门12');
    await page.getByRole('button', { name: '设置密码', exact: true }).click();
    await page.locator('.field-problem', { hasText: '不一样' }).waitFor();
    await page.getByLabel('再输入一次密码', { exact: true }).fill('开门123');
    await page.getByRole('button', { name: '设置密码', exact: true }).click();
    await page.getByRole('button', { name: '添加', exact: true }).waitFor();
    assert.deepEqual(callBridge('vault', 'status'), { exists: true, password: true });
    await page.getByRole('button', { name: '添加', exact: true }).click();
    const editor = page.getByRole('dialog', { name: '添加记录' });
    await editor.getByPlaceholder(/例如 QQ 邮箱/).fill('QQ 邮箱');
    await editor.getByLabel('邮箱/账号', { exact: true }).fill('me@qq.com');
    await editor.getByLabel('密码', { exact: true }).fill('p@ss');
    await shot('panel-records-editor.png');
    await editor.getByRole('button', { name: '保存', exact: true }).click();
    await page.locator('.record-row', { hasText: 'QQ 邮箱' }).waitFor();
    assert.equal(await page.locator('.record-field dd.masked').count(), 1);
    // An address, then the email renamed.
    await page.getByRole('button', { name: '添加', exact: true }).click();
    const address = page.getByRole('dialog', { name: '添加记录' });
    await address.getByRole('radio', { name: '地址' }).click();
    await address.getByPlaceholder(/例如 QQ 邮箱/).fill('家');
    await address.getByLabel('地址', { exact: true }).fill('北京市海淀区中关村大街 1 号');
    await address.getByRole('button', { name: '保存', exact: true }).click();
    await page.locator('.record-row', { hasText: '家' }).waitFor();
    await page.locator('.record-row', { hasText: 'QQ 邮箱' }).click();
    await page.getByRole('button', { name: '编辑 QQ 邮箱' }).click();
    await page.getByRole('dialog', { name: '编辑记录' }).getByPlaceholder(/例如 QQ 邮箱/).fill('工作邮箱');
    await page.getByRole('dialog', { name: '编辑记录' }).getByRole('button', { name: '保存', exact: true }).click();
    await page.locator('.record-row', { hasText: '工作邮箱' }).waitFor();
    await page.getByRole('tab', { name: '地址' }).click();
    assert.equal(await page.locator('.record-row').count(), 1);
    await page.getByRole('tab', { name: '全部' }).click();
    await shot('panel-records.png');
    const disk = fs.readFileSync(path.join(data, 'LocalPinyinIME', 'vault.json'), 'utf8');
    assert(!disk.includes('me@qq.com') && !disk.includes('工作邮箱'), 'records are encrypted on disk');
    // Locked: the password again.
    await page.getByRole('button', { name: '锁定', exact: true }).click();
    await page.getByRole('heading', { name: '记录已加密' }).waitFor();
    await page.getByLabel('记录密码', { exact: true }).fill('不对');
    await page.getByRole('button', { name: '打开', exact: true }).click();
    await page.locator('.toast.error', { hasText: '密码不对' }).waitFor();
    await page.getByLabel('记录密码', { exact: true }).fill('开门123');
    await page.getByRole('button', { name: '打开', exact: true }).click();
    await page.locator('.record-row', { hasText: '工作邮箱' }).waitFor();
    assert.equal(await page.locator('.record-row').count(), 2);
  });

  await check('dark theme', async () => {
    await page.emulateMedia({ colorScheme: 'dark' });
    await page.getByRole('button', { name: '符号', exact: true }).click();
    await page.waitForFunction(() => document.documentElement.dataset.theme === 'dark');
    await page.locator('.cell.symbol').first().click();
    await shot('panel-symbols-dark.png');
    await page.getByRole('button', { name: '表情', exact: true }).click();
    await shot('panel-emoji-dark.png');
  });

  assert.deepEqual(report.errors, []);
}

main().then(async () => {
  report.finishedAt = new Date().toISOString();
  fs.writeFileSync(path.join(output, 'panel-report.json'), JSON.stringify(report, null, 2));
  await app?.close();
  process.stdout.write(`All ${report.checks.length} panel checks passed.\n`);
}).catch(async error => {
  report.failure = String(error?.stack || error);
  fs.writeFileSync(path.join(output, 'panel-report.json'), JSON.stringify(report, null, 2));
  try { if (page) await page.screenshot({ path: path.join(output, 'panel-failure.png') }); } catch { /* window gone */ }
  await app?.close().catch(() => undefined);
  callBridge('clipboard-agent', 'stop');
  process.stderr.write(`${report.failure}\n`);
  process.exit(1);
});
