// Re-captures the product screenshots the website shows, from the real
// settings renderer (desktop/) with the Electron bridge mocked out.
//
//   cd desktop && npm ci && npx vite build --outDir ../build/site-screens
//   node site/scripts/capture_screens.mjs build/site-screens
//
// Needs Playwright with Chromium (`npm i -g playwright && npx playwright install chromium`).
// Writes site/assets/img/{settings,panel-tools}-{light,dark}.webp.
import { createServer } from 'node:http';
import { readFile, writeFile } from 'node:fs/promises';
import { extname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { chromium } from 'playwright';

const built = resolve(process.argv[2] ?? 'build/site-screens');
const { version } = JSON.parse(await readFile(new URL('../../desktop/package.json', import.meta.url), 'utf8'));
const images = fileURLToPath(new URL('../assets/img/', import.meta.url));
const types = { '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css', '.svg': 'image/svg+xml', '.png': 'image/png', '.woff2': 'font/woff2' };

const server = createServer(async (request, response) => {
  try {
    const path = new URL(request.url, 'http://local').pathname;
    const body = await readFile(join(built, path === '/' ? 'index.html' : decodeURIComponent(path)));
    response.writeHead(200, { 'content-type': types[extname(path)] ?? 'application/octet-stream' }).end(body);
  } catch {
    response.writeHead(404).end();
  }
});
await new Promise(ready => server.listen(0, '127.0.0.1', ready));
const origin = `http://127.0.0.1:${server.address().port}`;

// The settings app's bridge (desktop/electron/preload.ts): a plausible state,
// every other call resolves to something empty.
function mockSettings({ theme, version }) {
  // The first-run tour (desktop/src/Tour.tsx) counts as seen.
  localStorage.setItem('localpinyin.tour', 'done');
  const ok = value => Promise.resolve(value);
  const state = {
    version,
    settings: {
      theme, textSize: 4, candidateStyle: 'flow', typoCorrection: true, mixedEnglish: true, fuzzyPinyin: [], doublePinyin: 'none',
      usageDomains: ['daily', 'coding'], digitPeriod: true, digitPunctuation: true, letterPunctuation: false, codePunctuation: true,
      tabTranslate: false, panelDoubleTab: true, panelResident: false, emojiCandidates: true, englishAutocorrect: true,
      englishSentence: true, englishIme: true, imeSwitchKey: 'alt+capslock', clipboardHistory: 'on', clipboardRetentionDays: 30,
      shiftToggleEnabled: true, predictionEnabled: false,
    },
    dictionary: { total: 218942, core: 12480, local: 41250, standard: 165212 },
    learning: { count: 326, capacity: 2048 },
    runtime: { active: true, mode: 'zh', available: true },
    dataDirectory: 'C:\\Users\\you\\AppData\\Local\\LocalPinyinIME',
    userLexiconCount: 12,
  };
  const update = { state: 'current', current: state.version, autoCheck: true };
  const api = {
    initialState: state, getState: () => ok(state), saveSettings: () => ok(state),
    getUpdateStatus: () => ok(update), onUpdateStatus: () => () => {}, onSettingsChanged: () => () => {},
    domainWords: () => ok({ domains: { coding: [], work: [], math: [], study: [], daily: [], games: [] } }),
    clipboardSummary: () => ok({ items: 0, images: 0, bytes: 0 }),
    vaultStatus: () => ok({ exists: false, password: false, unlocked: false }),
  };
  window.localPinyin = new Proxy(api, { get: (target, key) => key in target ? target[key] : () => ok({ entries: [], phrases: [], total: 0 }) });
}

// The tool panel's bridge (desktop/electron/panel-preload.ts).
function mockPanel(theme) {
  const ok = value => Promise.resolve(value);
  const results = { '55+15': ['70'], '123': ['壹佰贰拾叁', '一百二十三'], '12.19': ['十二月十九日', '12月19日'] };
  const empty = { symbol: [], kaomoji: [], emoji: [] };
  const api = {
    bootstrap: { dark: theme === 'dark' },
    data: () => ok({ state: { favorites: empty, recents: empty }, stickers: [], dark: theme === 'dark', page: 'tools' }),
    tools: input => ok({ results: results[input] ?? [] }),
    onOpen: () => () => {},
  };
  window.localPinyinPanel = new Proxy(api, { get: (target, key) => key in target ? target[key] : () => ok({}) });
}

const browser = await chromium.launch();
async function capture(page, file, width) {
  const png = await page.screenshot();
  // Chromium encodes the WebP, so no image library is needed.
  const encoder = await browser.newPage();
  const data = await encoder.evaluate(async ({ source, width }) => {
    const image = new Image();
    image.src = source;
    await image.decode();
    const canvas = Object.assign(document.createElement('canvas'), { width, height: Math.round(image.height * width / image.width) });
    canvas.getContext('2d').drawImage(image, 0, 0, canvas.width, canvas.height);
    return canvas.toDataURL('image/webp', 0.84).split(',')[1];
  }, { source: `data:image/png;base64,${png.toString('base64')}`, width });
  await encoder.close();
  await writeFile(join(images, file), Buffer.from(data, 'base64'));
  console.log('wrote', file);
}

for (const theme of ['light', 'dark']) {
  const settings = await browser.newPage({ viewport: { width: 1160, height: 760 }, deviceScaleFactor: 2, colorScheme: theme });
  await settings.addInitScript(mockSettings, { theme, version });
  await settings.goto(`${origin}/index.html`);
  await settings.waitForTimeout(1500);
  await capture(settings, `settings-${theme}.webp`, 1920);
  await settings.close();

  const panel = await browser.newPage({ viewport: { width: 900, height: 600 }, deviceScaleFactor: 2, colorScheme: theme });
  await panel.addInitScript(mockPanel, theme);
  await panel.goto(`${origin}/panel.html`);
  await panel.waitForTimeout(1500);
  await capture(panel, `panel-tools-${theme}.webp`, 1500);
  await panel.close();
}
await browser.close();
server.close();
