# LocalPinyinIME Branding Assets

This directory contains first-party LocalPinyinIME brand assets for the “Trustworthy Tech Blue” concept.

## Icon

The app icon is a blue rounded Windows tile with a white speech bubble holding a blue "P" and a text caret above a keyboard. Its vector definition lives in `tools/generate_brand_icon.py`, which writes:

- `icon/LocalPinyinIME.svg` (vector mark)
- `icon/LocalPinyinIME.ico` (16, 20, 24, 32, 40, 48, 64, 128 and 256 px, each rasterized on its own pixel grid)
- `icon/LocalPinyinIME-master.png` and `icon/small/*.png`

See `icon/README.md` for regeneration.

## Theme Tokens

The canonical color token source is:

- `theme/local_pinyin_blue_theme.json`

The C++ header used by the app is generated into the build directory as `generated/local_pinyin_blue_theme.h`. Do not hand-edit the generated header.

Core palette:

- Deep Blue: `#0D47A1`
- Azure Blue: `#1976F3`
- Cyan Highlight: `#4DD0E1`
- Cool Gray: `#6B7280`
- Near White: `#F8FAFC`
