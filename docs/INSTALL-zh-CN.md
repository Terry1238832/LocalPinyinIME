# 安装与升级

请在解压后的 Release 目录中操作：

```text
@LOCALPINYIN_PACKAGE_NAME@
```

## 1. 管理员 PowerShell

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
cd <解压目录>\@LOCALPINYIN_PACKAGE_NAME@
```

## 2. 安装并加入当前用户输入法列表

推荐首次验证使用：

```powershell
.\scripts\Install-LocalPinyinIME.ps1 -EnableCurrentUser
```

安装脚本会复制文件到版本化目录：

```text
C:\Program Files\LocalPinyinIME\releases\@LOCALPINYIN_VERSION_PACKAGE@\x64\
```

随后调用：

```powershell
.\bin\LocalPinyinImeSetup.exe --register-system --dll "C:\Program Files\LocalPinyinIME\releases\@LOCALPINYIN_VERSION_PACKAGE@\x64\LocalPinyinIME.dll"
```

如果传入 `-EnableCurrentUser`，脚本会调用：

```powershell
.\bin\LocalPinyinImeSetup.exe --enable-current-user
```

该动作的含义是 `Enable LocalPinyinIME for current user`。它调用 `InstallLayoutOrTip(..., 0)`，不会调用 `SetDefaultLayoutOrTip`，不会激活输入法，也不会设置默认输入法。

## 3. 安装后的后置验证

`-EnableCurrentUser` 成功必须同时满足：

```text
1. 新 DLL 已复制到 release version 目录
2. InprocServer32 精确指向该 DLL
3. GetProfile 成功
4. EnumProfiles 包含 LocalPinyinIME
5. GUID_TFCAT_TIP_KEYBOARD 包含本 CLSID
6. IsEnabledLanguageProfile == TRUE
7. InputMethodTips contains LocalPinyinIME == TRUE
```

其中 `InputMethodTips` 检查来自：

```powershell
Get-WinUserLanguageList
```

并只检查：

```text
LanguageTag: zh-Hans-CN
InputMethodTips
```

## 4. 升级规则

升级不会覆盖当前正在被 TSF 使用的 DLL。脚本会：

```text
1. 读取当前 InprocServer32 路径
2. 确认旧路径位于 C:\Program Files\LocalPinyinIME\
3. 复制新版本到新的版本目录
4. 调用旧 DLL 的注销逻辑
5. 验证旧 COM/TIP 根已清理
6. 注册新版本 DLL
7. 验证 InprocServer32 指向新版本目录
```

如果当前注册路径指向未知外部位置，脚本会停止，避免加载不可信 DLL。

## 5. 只读验证

```powershell
.\scripts\Verify-LocalPinyinIME.ps1
```

最终能否在 `Win + Space` 中看到 LocalPinyinIME，以 `InputMethodTips contains LocalPinyinIME: TRUE` 和 Windows 输入法列表实际显示为准。
