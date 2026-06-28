# TSF Notes

The DLL uses real TSF interfaces from `msctf.h`:

- `ITfTextInputProcessorEx` for activation and deactivation.
- `ITfThreadMgr` and `ITfKeystrokeMgr` for key event sink registration.
- `ITfKeyEventSink` for key handling.
- `ITfContext`, `ITfInsertAtSelection`, `ITfContextComposition`,
  `ITfComposition`, and `ITfRange` for composition updates.
- `ITfInputProcessorProfiles` for text service and language profile
  registration.

Composition operations are performed inside synchronous read/write edit
sessions. Every HRESULT returned by TSF calls is checked and logged at the
boundary.
