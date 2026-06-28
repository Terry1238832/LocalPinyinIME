# Uninstall Guide

Open PowerShell as administrator and run:

```powershell
.\scripts\unregister_ime.ps1 -Configuration Debug
```

The user data directory is not deleted by the unregister script:

```text
%LOCALAPPDATA%\LocalPinyinIME
```

Remove it manually only when you want to delete local learning data and logs.
