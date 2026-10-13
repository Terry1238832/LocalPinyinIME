# 安全说明

本地拼音 0.7.2 是 Windows x64 未签名构建。安装需要管理员授权；当前版本尚未完成干净机器上的完整安装、卸载和逐款应用验收，验证范围见 [应用兼容性说明](COMPATIBILITY.md)。

## 安装和个人数据

安装器将原生输入核心、设置桥接、注册助手、五份输入数据，以及Electron设置程序和运行时安装到版本目录，并保留项目和运行时许可证：

```text
C:\Program Files\LocalPinyinIME\releases\0.7.2\x64\
```

安装器注册本项目自己的 COM CLSID 和 TSF 中文语言 Profile，并通过原始用户执行启用及验证，将本地拼音加入当前用户可选输入法。该启用步骤会自动执行；安装后仍需按 Win + 空格选择本地拼音。注册失败时，安装器尝试恢复原有注册。

```text
COM CLSID: {7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}
TSF Profile: {84D58E7C-481E-4D20-A951-4ED39F01D8D5}
LANGID: 0x0804
```

个人设置、词库和学习记录保存在 `%LOCALAPPDATA%\LocalPinyinIME`。输入与学习在本地处理，不发送网络请求。卸载保留个人数据，并且仅在注册仍指向该版本时移除本项目的 TSF 注册。

安装器不设置默认输入法，不修改其他输入法的注册，不安装服务、计划任务或开机启动项。输入核心使用当前应用线程的键盘事件，不安装全局键盘Hook或使用SendInput；不轮询剪贴板。设置文本框保留正常粘贴操作。

Electron只加载打包的本地资源，renderer禁用Node并开启sandbox/contextIsolation。preload仅提供固定设置API，主进程校验消息来源和参数，再通过无shell原生桥接操作已知个人数据目录。外部导航、弹出窗口和远程资源请求被拒绝。关闭设置后Electron退出。

## 哈希与签名

发布的安装包及对应校验文件为：

```text
LocalPinyinIME-0.7.2-x64.exe
LocalPinyinIME-0.7.2-x64.exe.sha256
```

校验文件包含安装包的 SHA-256 及文件名。可以在下载目录执行以下只读命令，将结果与 `.exe.sha256` 文件中的哈希比较：

```powershell
Get-FileHash -LiteralPath .\LocalPinyinIME-0.7.2-x64.exe -Algorithm SHA256
```

哈希不一致时不要运行该安装包。哈希用于核对下载文件；本版本没有 Authenticode 签名。
