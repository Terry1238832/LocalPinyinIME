# Known Limitations

- The TSF DLL compiles against real Windows APIs, but this workspace has not
  performed a live Windows Settings registration and Notepad/Chrome/Word input
  verification pass.
- Candidate window placement currently uses the mouse cursor as a fallback
  anchor. The next pass should query the TSF text range bounding box so it can
  follow the caret precisely.
- `ITfCandidateListUIElement` is not yet exposed. The current candidate UI is a
  native non-activating Win32 popup.
- SQLite is represented by an adapter boundary, but the current tests use a
  local file-backed learning store to avoid requiring a sqlite3 SDK in the
  build environment.
- Password/sensitive field detection is not complete. The code avoids logging
  input content, but InputScope-based learning suppression still needs to be
  wired into the TSF context path.
- PageUp/PageDown candidate paging is reserved for the next milestone.
