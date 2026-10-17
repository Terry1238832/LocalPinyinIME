# LocalPinyinIME Branding Assets

This directory contains first-party LocalPinyinIME brand assets for the “Trustworthy Tech Blue” concept.

## Icon

The app icon is a blue rounded Windows tile with a white speech bubble holding a blue "P" and a text caret above a keyboard. Its vector definition lives in `tools/generate_brand_icon.py`, which writes:

- `icon/LocalPinyinIME.svg` (vector mark)
- `icon/LocalPinyinIME.ico` (16, 20, 24, 32, 40, 48, 64, 128 and 256 px, each rasterized on its own pixel grid)
- `icon/LocalPinyinIME-master.png` and `icon/small/*.png`

See `icon/README.md` for regeneration.

## Design Tokens

`theme/design_tokens.json` is the single source for colors, radii, type and motion
across the settings app, the candidate window and the installer. Run
`tools/generate_design_tokens.py` to regenerate `desktop/src/tokens.css`; CMake
generates `localpinyin_design_tokens.h` for the native surfaces at configure time.
Do not hand-edit either output. `docs/DESIGN_SYSTEM.md` describes how the tokens
are used.

Brand colors: Azure `#1976F3` (the icon and brand mark) and Deep Blue `#0D47A1`.
Interface fills use the deeper accent `#1A6FE8` (light) / `#2A6FE0` (dark) so white
text keeps a 4.5:1 contrast.
