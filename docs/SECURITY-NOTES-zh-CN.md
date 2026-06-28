# 安全说明

LocalPinyinIME @LOCALPINYIN_VERSION_PACKAGE@ x64 是未签名开发版，不是正式签名发布版。

## 本包会做什么

- 安装到版本化目录：

```text
C:\Program Files\LocalPinyinIME\releases\<version>\x64\
```

- 注册本项目自己的 COM CLSID：

```text
{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}
```

- 注册本项目自己的 TSF Language Profile：

```text
{84D58E7C-481E-4D20-A951-4ED39F01D8D5}
```

- 使用 LANGID：

```text
0x0804
```

- 可选地把本项目 Profile 加入当前用户输入法列表。

## 本包不会做什么

- 不设置默认输入法；
- 不调用 `SetDefaultLayoutOrTip`；
- 不调用 `ActivateProfile`；
- 不修改 `Preload`、`Substitutes`、`Keyboard Layout`；
- 不修改 Microsoft Pinyin、搜狗、QQ、微信输入法或其他输入法；
- 不使用全局键盘 Hook；
- 不使用 `SendInput`；
- 不读取剪贴板；
- 不发送网络请求；
- 不安装服务、计划任务或开机启动项；
- 不直接写入用户语言列表或伪造 `InputMethodTips`。

## 当前用户列表验证

`IsEnabledLanguageProfile` 只能证明 TSF Profile enabled / disabled，不能单独证明 `Win + Space` 可见。

最终以只读检查为准：

```text
Get-WinUserLanguageList
-> zh-Hans-CN
-> InputMethodTips
```

必须包含精确 TIP：

```text
0804:{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}{84D58E7C-481E-4D20-A951-4ED39F01D8D5}
```

## 哈希与签名

Release 包包含 `SHA256SUMS.txt`。当前 Authenticode 状态应为：

```text
NotSigned
```

如果文件哈希与 `SHA256SUMS.txt` 不一致，请不要安装。
