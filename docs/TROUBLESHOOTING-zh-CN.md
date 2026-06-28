# 故障排查

## Win + Space 看不到 LocalPinyinIME

先运行只读验证：

```powershell
.\scripts\Verify-LocalPinyinIME.ps1
```

重点看：

```text
Current user:
- IsEnabledLanguageProfile: TRUE/FALSE
- InputMethodTips contains LocalPinyinIME: TRUE/FALSE
```

如果 `IsEnabledLanguageProfile == TRUE`，但 `InputMethodTips contains LocalPinyinIME == FALSE`，则说明 TSF Profile 状态和当前用户输入法列表不一致，不能认为已经可在 `Win + Space` 中使用。

## 手动启用当前用户

确认系统注册存在后，可以运行：

```powershell
.\bin\LocalPinyinImeSetup.exe --enable-current-user
.\scripts\Verify-LocalPinyinIME.ps1
```

该命令不会设置默认输入法。只有 `InputMethodTips contains LocalPinyinIME: TRUE` 后，才表示当前用户列表包含该 TIP。

## Notepad 无法输入

请确认使用 Windows 官方 `notepad.exe`，并手动切换到：

```text
LocalPinyinIME - 离线拼音输入法
```

基础测试：

```text
nihao + Space -> 你好
wo + Space -> 我
Esc -> 取消组合
Ctrl + Space -> 英文直输
```

## 安装脚本失败

脚本会输出：

```text
Step
Command
HRESULT
Win32 error
ExitCode
InputMethodTips contains LocalPinyinIME
```

失败时不要手动广泛清理注册表。请保留安装目录与日志。

## 未签名提醒

当前包是 `unsigned development release`。Windows 安全提醒是预期现象，不代表这是正式签名发行版。
