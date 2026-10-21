# 本地拼音官网 — DESIGN.md

The spec for `site/` (GitHub Pages). Written with the project skills `product-launch-page`,
`web-design` (Phase B, interaction tier L3) and `frontend-design`. Candidate-window colors come
only from `assets/branding/theme/design_tokens.json` through
`site/scripts/candidate_tokens.py`; everything else is defined here.

## 0. Brief

- **Product**: 本地拼音 0.23.0, an offline Chinese pinyin input method for Windows 10/11, with a
  second offline English input method (本地英文). Free, MIT.
- **Who**: Chinese-speaking Windows users who type all day and do not want a cloud input method
  reading their keystrokes, showing ads or popping up news; developers who mix Chinese, English
  and code.
- **The page's job**: make "smart *and* local" believable in one scroll, show the candidate window
  (the product's face) as the hero object, and end at a download button that tells the truth
  (preview release, 112 MB, Windows x64).
- **Evidence only from the repo**: README release notes, `docs/*.md`, design tokens, real
  screenshots of the 0.23 settings app and tool panel rendered from `desktop/` with a mocked bridge.

## 1. Visual theme and atmosphere

A product theatre. Each chapter is one claim, one piece of evidence, one moment of motion. The
product is a small floating window, so the page makes it large: the candidate window is drawn
live at 1.5–2× in the product's own palettes, never as a bitmap.

- Keywords: precise, quiet, confident, typed.
- Signature: the input method's own vocabulary. The blue text caret (the `P|` of the icon) closes
  the hero title; the hero types `ben'di'pin'yin` above its title and then `ni'hao'shi'jie` into a live window that lists 你好世界; the download
  chapter types `xia'zai` and commits 下载. Pinyin separators are a step lighter, as in the IME.
- Rejected: aurora / mesh backgrounds, gradient text, glass stacks, three-column feature cards,
  logo walls, tracked-out caps eyebrows, custom cursors, WebGL (the product is not a 3D object).

## 2. Color palette and roles

The page follows the system theme and has a manual switch (auto / light / dark, stored per viewer).
The manifesto and candidate-style chapters are always dark — a stage — in both themes.

| Token | Light | Dark | Role |
| --- | --- | --- | --- |
| `--bg` | `#FFFFFF` | `#000000` | page |
| `--bg-alt` | `#F5F5F7` | `#161618` | alternate chapters |
| `--fg` | `#1D1D1F` | `#F5F5F7` | text |
| `--fg-2` | `#6E6E73` | `#A1A1A6` | secondary text (4.9:1 / 7.9:1) |
| `--rule` | `rgb(0 0 0 / .12)` | `rgb(255 255 255 / .16)` | hairlines |
| `--accent` | `#1A6FE8` | `#2A6FE0` | buttons (white text ≥ 4.5:1) |
| `--link` | `#1A64D6` | `#5EA1FF` | text links |
| `--caret` | `#1976F3` | `#5A97FF` | brand caret |

Stage (always dark): `#000000` ground, `#F5F5F7` text, `#86868B` dim words.
Candidate palettes: `--cand-*` per `[data-cand-style][data-cand-tone]`, generated.
No other hues. Amber, yellow and vermilion appear only because three candidate styles use them.

## 3. Typography

System fonts only, so the page sends no request to a font host (and looks like the OS the product
runs on): Latin from SF Pro / Segoe UI Variable, Chinese from PingFang SC / Microsoft YaHei UI /
Noto Sans SC.

```
--font: -apple-system, BlinkMacSystemFont, "SF Pro Text", "Segoe UI Variable Text", "Segoe UI",
        "PingFang SC", "Microsoft YaHei UI", "Microsoft YaHei", "Noto Sans SC", system-ui, sans-serif;
--font-display: -apple-system, BlinkMacSystemFont, "SF Pro Display", "Segoe UI Variable Display",
        "Segoe UI", "PingFang SC", "Microsoft YaHei UI", "Microsoft YaHei", "Noto Sans SC", system-ui, sans-serif;
```

| Role | Size | Weight | Line height | Tracking |
| --- | --- | --- | --- | --- |
| Hero title | clamp(64px, 12vw, 144px) | 600 | 1.02 | −0.01em |
| Chapter title | clamp(40px, 6.4vw, 80px) | 600 | 1.08 | −0.005em |
| Manifesto | clamp(30px, 4.6vw, 60px) | 600 | 1.32 | 0 |
| Lead | clamp(19px, 1.9vw, 24px) | 500 | 1.6 | 0.01em |
| Body | 17px | 400 | 1.75 | 0.02em |
| Small / footnote | 13–14px | 400 | 1.6 | 0.02em |
| Big number | clamp(48px, 7vw, 88px) | 600 | 1 | −0.01em, tabular-nums |

Chinese body ≥ 17px, line height ≥ 1.7. Line length ≤ 36 Chinese characters (≈ 640px).

## 4. Components

- **Nav**: 48px, sticky, `color-mix(--bg 78%)` + `backdrop-filter: saturate(180%) blur(12px)`;
  icon + 本地拼音, chapter links (hidden < 860px), theme switch, pill 下载.
- **Buttons**: pill, 44px min height, 17px/500. Primary = `--accent` fill, white text; hover
  lightens 6%, active scale .97, focus = 3px ring `--caret` at 2px offset; disabled 40% opacity.
  Secondary = text link with `--link`, underline on hover.
- **Candidate window** (`.cand`): built from markup + `--cand-*`; eight variants (§7).
- **Window frame** (`.frame`): screenshots sit in a 10px-radius frame with a 1px rule and a soft
  photographic shadow `0 30px 80px -20px rgb(0 0 0 / .35)`.
- **Keycap** (`.key`): 1px rule, 2px bottom edge, 8px radius, presses by translateY(2px).
- **Ruled list** (`.ruled`): the settings app's ruled sheet — hairline above each row, title 19/600,
  text 15/400 `--fg-2`; two columns ≥ 860px.
- **Style picker**: radio group (roving focus, arrow keys); each option is a name and its English
  name; the checked one is underlined by a 2px bar in its own `--cand-focus-bar`.

## 5. Layout

- Container 1120px max, side gutter clamp(20px, 5vw, 48px). Chapters pad clamp(96px, 14vh, 180px)
  top and bottom; heroes fill `100svh`.
- Centered headings for the theatre chapters, left-aligned copy blocks for reading chapters
  (typing, details, download). No section is a grid of equal cards.

## 6. Depth

Flat chapters, one floating thing at a time: the candidate window (its own native shadows,
`shadow-key-alpha` and `shadow-ambient-alpha` from the tokens) and screenshot frames.

## 7. Chapters, content and motion (L3)

| # | Chapter | Claim | Evidence | Motion |
| --- | --- | --- | --- | --- |
| 1 | Hero | 本地拼音。聪明的输入法，只在你的电脑上思考。 | the Adaptive Flow window typing `ni'hao'shi'jie` → 你好世界 | load: the title's pinyin types and commits, then the window types `ni'hao'shi'jie` once (drop pours left→right, changed candidates float up) and rests composed — under 5 s of autoplay. Scroll-out: window rises and scales 1→1.06, title fades |
| 2 | Manifesto (stage) | 你打的每一个字，都在这台电脑上读成候选、排序、记住。没有云端，没有账号，没有广告。 | three facts + an honest footnote (update check, Tab translation, sticker URLs) | **sticky**; words light from dim to white with scroll progress |
| 3 | Context | 它会看前文。 | 他跑 + de → 得, 没有 + shijian → 时间, 这种又是 + zalaide → 咋来的; 86.04% / 1,360 万字 / 17.1 MB | **sticky** three-step scene: the line before the caret changes, the candidate list re-ranks, the focus slides |
| 4 | Typing | 打错了，它也认得。 | nihso → 你好, woyongpythonxiedaima → 我用 Python 写代码, bjdx → 北京大学, zongguo → 中国, nihc → 你好, yilaizhuru → 依赖注入, liangjiacheng → 量甲成 | each row types its pinyin, then commits the result in place (one-shot, staggered) |
| 5 | Styles (stage) | 八种候选框，一套按键。 | live window in all eight styles, light and dark | reveal: pour; switching: crossfade + the style's own move (drop slides, flaps flip, marker strokes, capsule slides) |
| 6 | Settings | 设置，像一张划了格线的纸。 | real 0.23 外观 page, light/dark by theme | product zoom: frame scales .88→1 and settles while the chapter crosses the viewport |
| 7 | Tool panel | 连按两下 Tab。 | Tab Tab keycaps, real 工具 page; 3,100 符号, 1,000 颜文字, ~1,870 表情, 剪贴板, 表情包, 记录 | keycaps press in turn, then the panel rises |
| 8 | English | 还有一个英文输入法。 | `i think teh plan could of worked` → `I think the plan could have worked`; ~13 万词; 86.0% | sentence types, Space Space press, the four fixes swap in with a short underline |
| 9 | Details | 还有这些细节。 | ruled list of ten verified details | none beyond the shared reveal |
| 10 | Download | 下载 | 0.23.0 preview, 112 MB, Windows 10/11 x64, SHA-256, three steps | `xia'zai` types and commits |

Scroll engine: one rAF loop reads `getBoundingClientRect()` of the active sticky scenes and writes
CSS custom properties (`--p`); IntersectionObserver starts one-shot reveals and pauses loops off
screen. No library.

## 8. Do's and Don'ts

Do
1. Draw the product live from its tokens; screenshots only of real screens.
2. Keep one claim per chapter, and back it with a number, a demo or a screen.
3. Footnote every exception to "local".
4. Animate only transform, opacity and clip-path.
5. Make every demo readable without JS and without motion.
6. Use the product's own words (候选, 上屏, 空格, 前文) and its key names.

Don't
1. No gradients as decoration, no gradient text, no glow.
2. No equal-card grids, logo walls, testimonial strips or fake social proof.
3. No numbers that are not in the repo's docs.
4. No third-party requests: no web fonts, analytics, CDNs or embeds.
5. No `filter: blur()` animation, no scroll-jacking, no custom cursor.
6. No all-caps eyebrows or `A · B · C` meta strings.
7. No Apple marks, product names, copy or imagery.
8. No more than two sticky scenes.

## 9. Responsive and accessibility

- Breakpoints: ≤ 600 (phone), 601–1023 (tablet), ≥ 1024 (desktop). Candidate window scales by
  `--cand-scale`: 2 desktop, 1.5 tablet, 1.15 phone; wide styles wrap their chips on phones.
- Sticky scenes: 260svh desktop/tablet, 220svh phone; with `prefers-reduced-motion: reduce` they
  become ordinary stacked content and every demo shows its end state.
- Low-power devices (`saveData`, ≤ 4 cores or ≤ 4 GB): hero parallax and scroll-scrubbed scaling
  are off. Carets blink five times and then stay lit.
- Skip link, landmarks, one `h1`, chapter `h2`s, visible focus, 44px targets, `aria-hidden` on
  decorative demos with a text equivalent next to each, `lang="zh-CN"`.

## 10. Maintaining

- **Preview**: serve `site/` over HTTP (`npx http-server site` or `python -m http.server -d site`)
  and open it; `file://` works too, but test over HTTP.
- **Candidate palettes**: after editing `design_tokens.json`, run
  `python site/scripts/candidate_tokens.py`. The workflow fails while the file is out of date.
- **Screenshots**: when the settings app or tool panel changes, rebuild the renderer and
  re-capture (Playwright with Chromium required):
  `cd desktop && npx vite build --outDir ../build/site-screens`, then
  `node site/scripts/capture_screens.mjs build/site-screens`. The script mocks the Electron
  bridge, so no native build is needed.
- **Release links**: the download chapter links the exact installer of a tagged release (GitHub's
  `/releases/latest` skips pre-releases, and every release so far is one). Update the version,
  file name and size in `index.html` with each release.
- **Deploy**: `.github/workflows/pages.yml` publishes `site/index.html` and `site/assets/` on
  every push to `main` that touches the site. Pages must be set to *Source: GitHub Actions* once
  in the repository settings.
