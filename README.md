# 本地拼音 0.4.0

Windows 10/11 x64 原生 TSF 离线输入法。使用 C++20、Win32 和系统 COM 接口，运行不需要网络、Python、浏览器或额外运行库。

支持全拼、连续拼音与整句候选、简拼（例如 nh → 你好）、拼音补全、单次拼写错误容错，以及跨应用持久保存的选词学习。词库约 2.18 万条，保存在两份 UTF-8 TSV 文件中。用户可在设置内添加、搜索、编辑和删除个人词条。

设置页提供明暗主题、跟随系统、候选字号、实时预览、中英文切换设置、词库管理、学习记录清空和使用帮助。修改后自动保存。

从 [GitHub Release v0.4.0](https://github.com/Terry1238832/LocalPinyinIME/releases/tag/v0.4.0) 下载 [Windows x64 安装包](https://github.com/Terry1238832/LocalPinyinIME/releases/download/v0.4.0/LocalPinyinIME-0.4.0-x64.exe)（约 2.93 MB）及同页的 SHA-256 文件。双击安装，完成后按 Win + 空格选择“本地拼音”。空格选择首项，数字 1–9 选词，左右方向键移动候选，Esc 取消，Enter 提交原始拼音，Ctrl + Space 切换中英文。

![本地拼音设置界面](docs/images/interface.png)

## 构建

需要 Visual Studio C++ 构建工具及 Windows SDK、CMake/Ninja、Python 3（仅用于生成主题和验证词库），以及 Inno Setup 6。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build.ps1
```

脚本自动发现本机工具，构建 Release、运行回归测试，并在仓库内的 `dist/` 生成安装包和 SHA-256 文件。也可传入 `-CMakePath`、`-PythonPath`、`-IsccPath`。不下载依赖。临时产物只放在 `.build` 内，发布后可删除整个 `.build`。

源码分为 `src/engine`（词库、解码、排序和学习）、`src/ime`（系统 TSF 与候选窗）、`src/settings`（设置界面）、`src/setup`（注册与验证）和 `src/common`（系统与编码辅助）。共享代码只编译一次供产品和测试复用。

## 数据与验证范围

个人数据位于 `%LOCALAPPDATA%\LocalPinyinIME`，包括 `settings.json`、`user_lexicon.tsv` 和 `user_learning.tsv`。学习最多保留 2,048 条记录；常规输入不写诊断日志。设置里的“清空学习记录”保留个人词库。卸载保留个人数据。

测试覆盖词库、全拼/简拼/容错、整句候选、学习持久化和并发更新、DPI 布局、TSF 生命周期及 COM DLL。TSF 集成测试使用真实 Windows 文档/上下文/编辑会话，键盘注册通过测试适配器隔离；不修改当前输入法注册。因此该测试不等同于逐款应用的安装后兼容性测试。

本版本为未签名的 x64 本地构建。32 位应用尚未支持。Microsoft Store 等部分沉浸式宿主的自绘候选窗仍存在兼容性限制。主机应用行为可能不同；出现无法输入时可按 Win + 空格切回其他输入法并重新打开应用。

## 许可

代码采用 MIT 许可，见 LICENSE。内置词库是项目维护的第一方内容；分发与来源记录见 `resources/dictionary/sources/lexicon_sources.json`，词条统计保留在词库元数据内。
