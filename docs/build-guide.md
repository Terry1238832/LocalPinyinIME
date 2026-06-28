# Build Guide

Open a Visual Studio 2022 Developer PowerShell, then run:

```powershell
cd LocalPinyinIME
.\scripts\build_debug.ps1
```

This configures CMake for x64, builds the TSF DLL, builds the settings app, and
runs the engine/data tests.

Release build:

```powershell
.\scripts\build_release.ps1
```
