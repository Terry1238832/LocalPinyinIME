# 本地拼音 settings — interface system

The full spec is [docs/DESIGN_SYSTEM.md](../docs/DESIGN_SYSTEM.md); tokens come only from
`assets/branding/theme/design_tokens.json` (regenerate `desktop/src/tokens.css` with
`python tools/generate_design_tokens.py --css desktop/src/tokens.css`). This file keeps the
decisions in short form for design passes.

## Direction

- Who: a Windows user who stopped typing to change one thing (a candidate style, a word, a
  switch key) and wants to get back to typing. Operate mode: scanability and native
  expectations over expression.
- Feel: a ruled sheet of paper. Warm paper (`#F4F3EF`) and blue-black ink; azure only for
  state. Swiss: grid, hairlines, weight-led hierarchy, no decoration.
- Signature: the input method's own vocabulary.
  - Sidebar = a candidate list numbered 1–7 (Ctrl+1–7); the current page is drawn in the
    user's chosen candidate style (`nav[data-cand-style]` takes `--cand-focus*`).
  - Page title = a composition: pinyin with a dotted underline and the brand caret, then the
    committed title.
  - Command palette = a candidate window: numbered results, digits 1–9 run them.
  - Sidebar footer = the language bar's 中 / 英 tile.
  - Illustrations and empty states use `MiniCandidates`.
- Rejected defaults: icon + label nav with a white pill; a white card per settings group;
  ambient glows, grain and glass; gradient primary buttons with glows; spring easing.

## Depth and surfaces

- One strategy: hairlines plus one surface step. Canvas = paper. Things you work with
  (preview stage, word lists, the English IME card, small detail sheets) = `surface` with a
  1 px `--rule` border and `--shadow-xs`. Real shadows only on floating things (dialogs,
  palette, toast, candidate previews).
- Rules: the first line of a section is `--rule-strong`; rows are separated by `--rule`;
  no closing line under a section.

## Spacing and type

- 4 px grid. Page sections 44 apart. Margin column 196 (`--margin-column`) + 40 gap
  (`--margin-gap`). Rows min 56 high, 11 vertical padding. Page frame max 1080, side padding
  clamp(28px, 5cqi, 72px).
- Segoe UI Variable Display / Text with Microsoft YaHei UI. Scale: 11 caption · 12 · 13 body-sm
  · 14 body · 15 section title (600) · 18 · 28 page title (600, −0.02 em) · 36 metric · 52 hero.
  Row names 14/500; descriptions 13 `text-2`; meta `text-3` (light `#6B6E76`, 4.6:1 on paper).
- Numbers in counts and tables: `tabular-nums`.

## Components (values)

- Nav item: 36 h, radius 8, number 11/600 via CSS `content: attr(data-number) / ""`.
- Button: 32 h, radius 8, 13/500; primary flat `--accent`; secondary white, `--rule-strong`
  border, `--shadow-xs`; press scale .97.
- Switch: 40×24, thumb 20, on = `--accent`, no halo; 260 ms `--ease-out-quint`.
- Segmented: `--fill-hover` track, 2 px inset, items 28 h; active white + `--shadow-xs`, 600.
- Search field: `--fill-hover` inset, border on hover, white on focus, 3 px focus ring.
- Detail / disclosure panel: a small sheet, radius 12, 14–16 padding.
- Key reference: ruled rows, action left 13 px, keys right; 2 columns (3 at ≥1180 content).

## Motion

100 ms color/press · 160 ms hover, page fade-in, disclosure · 260 ms nav mark, dialogs,
switch. Curve `cubic-bezier(0.23, 1, 0.32, 1)`. Everything off under reduced motion.
