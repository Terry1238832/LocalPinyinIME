# LocalPinyinIME Icon Assets

The application icon is the approved Concept A design: a blue rounded tile,
a white speech bubble holding a blue "P" and a text caret, and a keyboard. Its single source of truth is the vector definition in
`tools/generate_brand_icon.py`; every file in this directory is generated from
it:

- `LocalPinyinIME.ico` — 16, 20, 24, 32, 40, 48, 64, 128 and 256 px entries,
  written as uncompressed 32-bit BGRA DIBs with an alpha-derived AND mask so the
  resource compiler and `LoadImageW` keep transparency in title bars and the
  taskbar.
- `LocalPinyinIME-master.png` — 1024 px preview.
- `LocalPinyinIME.svg` — vector mark used by the settings interface.
- `small/LocalPinyinIME-<n>.png` — 16–40 px renders.

Each size is rasterized on its own with 8x supersampling. Sizes of 48 px and
up draw the full design; smaller sizes omit the keyboard, enlarge the bubble
and snap the P to whole-pixel strokes, so they stay sharp rather than being
downscaled from the large master.

Regenerate with:

```powershell
python .\tools\generate_brand_icon.py --output-dir .\assets\branding\icon
```

The taskbar 中/英 indicator is drawn at runtime in the taskbar's text color
(`src/ime/language_bar.cpp`). The fallback resources in `assets/mode-icons`
come from `tools/generate_mode_icons.py`.

The installer icon is configured in `installer/LocalPinyinIME.iss` with
`SetupIconFile=..\assets\branding\icon\LocalPinyinIME.ico`. The Start Menu
Settings and Uninstall shortcuts use `{app}\LocalPinyinSettings.exe` as their
icon source.
