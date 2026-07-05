# LocalPinyinIME Icon Assets

`LocalPinyinIME-master.png` is the approved Concept A production master.

The production Windows icon `LocalPinyinIME.ico` must be generated from
`LocalPinyinIME-master.png` for 48, 64, 128, and 256 px entries. The 16, 20,
24, 32, and 40 px entries must come directly from the cleaned design-provided
PNG files in `small/`.

The uploaded design originals are preserved in `small/source/`. They are passed
through the local generator to remove edge-connected white matte/background
pixels before being written to `small/`. This keeps the small icons legible on
dark taskbars without introducing a separate redraw.

All ICO entries are written as uncompressed 32-bit BGRA DIB images with an
alpha-derived AND mask. This is intentional: the Windows resource compiler and
`LoadImageW` path used by `LocalPinyinSettings.exe` must preserve transparency
for title-bar and taskbar icons, not only for standalone PNG inspection.

The SVG files in this directory are development wrappers around the approved PNG
master and are not the production source for the ICO.

Do not redraw, simplify, vectorize, or restyle the icon when regenerating the
Windows icon resource.

Regenerate the icon and preview sheets with:

```powershell
python .\tools\generate_brand_icon.py `
  --output-dir .\assets\branding\icon `
  --master .\assets\branding\icon\LocalPinyinIME-master.png `
  --small-assets-dir .\assets\branding\icon\small `
  --small-source-dir .\assets\branding\icon\small\source `
  --light-contact-sheet .\build-small-icon-source-assembly\branding-icon-contact-sheet-light.png `
  --dark-contact-sheet .\build-small-icon-source-assembly\branding-icon-contact-sheet-dark.png
```

The installer icon is configured in `installer/LocalPinyinIME.iss` with
`SetupIconFile=..\assets\branding\icon\LocalPinyinIME.ico`. The Start Menu
Settings and Uninstall shortcuts use `{app}\LocalPinyinSettings.exe` as their
icon source.
