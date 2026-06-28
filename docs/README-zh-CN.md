# LocalPinyinIME @LOCALPINYIN_VERSION_PACKAGE@ x64 开发版

LocalPinyinIME 是一个本地离线 Windows 中文拼音输入法。它是原生 TSF/COM text service，不是全局键盘 Hook、网页输入框、AutoHotkey 脚本，也不使用 SendInput 或剪贴板模拟输入。

本 Release 包是 `unsigned development release`，仅包含 x64 组件。不声明支持 32 位应用；后续如需 32 位应用兼容，需要单独构建 x86 DLL 并并行安装。


## Dictionary resource

Release packages must contain:

```text
bin\dictionary\core_zh_pinyin.tsv
```

`validEntries` means the number of unique TSV entries that parse successfully, have non-empty pinyin and word fields, have a positive integer frequency, and are not a duplicate of the same `pinyin + word` pair. The release manifest keeps `sourceRows`, `commentRows`, `blankRows`, `duplicateRows`, `invalidRows`, and `validEntries` separate.
## 安装前须知

- 系统级 TSF 注册需要管理员权限。
- 安装不会设置默认输入法，也不会自动切换当前输入法。
- 如需让 `Win + Space` 可见，需要把本输入法加入当前用户输入法列表。
- `Win + Space` 可见性的只读验证以 `Get-WinUserLanguageList -> zh-Hans-CN -> InputMethodTips` 是否包含精确 TIP 为准：

```text
0804:{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}{84D58E7C-481E-4D20-A951-4ED39F01D8D5}
```

- 仍可能需要安装 Microsoft Visual C++ Runtime。
- 当前 DLL 未签名，不应作为正式签名发行版分发。

## 基础测试

安装、启用当前用户并手动切换到 LocalPinyinIME 后，在 Windows 官方 `notepad.exe` 中测试：

```text
nihao + Space -> 你好
wo + Space -> 我
Esc -> 取消组合
Ctrl + Space -> 英文直输
```

如果其他应用异常，请先回到系统自带 `notepad.exe` 验证基础行为。

## 日志位置

```text
%LOCALAPPDATA%\LocalPinyinIME\logs\status.log
```

日志不应记录用户实际输入内容、候选词、剪贴板或密码字段内容。
