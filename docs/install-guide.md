# Install Guide

1. Build the project.
2. Open PowerShell as administrator.
3. Run:

```powershell
.\scripts\register_ime.ps1 -Configuration Debug
```

4. Open Windows Settings -> Time & language -> Language & region.
5. Add or enable the Chinese input method named
   `LocalPinyinIME - 离线拼音输入法`.

If registration fails, check `%LOCALAPPDATA%\LocalPinyinIME\logs\status.log`.
