# LocalPinyinIME Branding Assets

This directory contains first-party LocalPinyinIME brand assets for the “Trustworthy Tech Blue” concept.

## Icon

The app icon uses a blue rounded Windows tile, a pinyin speech bubble mark, and a compact keyboard motif. The canonical editable sources are:

- `icon/LocalPinyinIME.svg`
- `icon/LocalPinyinIME-small.svg`

The Windows icon file is generated from the same design language:

- `icon/LocalPinyinIME.ico`

The ICO contains 16, 20, 24, 32, 40, 48, 64, 128, and 256 px images. The smallest sizes use a simplified mark to avoid unreadable detail.

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
