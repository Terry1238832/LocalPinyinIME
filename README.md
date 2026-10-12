# 本地拼音 0.5.1

Windows 10/11 x64 原生 TSF 离线输入法。使用 C++20、Win32 和系统 COM 接口，运行不需要网络、Python、浏览器或额外运行库。

支持全拼、连续拼音与整句候选、简拼（例如 nh → 你好）、拼音补全、单次拼写错误容错，以及跨应用持久保存的选词学习。词库83,062 条，配合小型本地字符上下文模型组合整句；可先选前部词语，再继续编辑剩余拼音。用户可在设置内添加、搜索、编辑和删除个人词条；建议频率5000的长个人词在完整拼音匹配时优先出现。

设置页采用原生 Direct2D/DirectWrite 绘制，提供明暗主题、跟随系统、候选字号、真实候选预览、中英文切换设置、个人词库管理、内置词库浏览、学习记录清空和使用帮助。修改后自动保存；支持键盘导航及原生开关语义。候选窗支持鼠标选词，中文模式自动输入中文标点。

从 [GitHub Release v0.5.1](https://github.com/Terry1238832/LocalPinyinIME/releases/tag/v0.5.1) 下载 [Windows x64 安装包](https://github.com/Terry1238832/LocalPinyinIME/releases/download/v0.5.1/LocalPinyinIME-0.5.1-x64.exe)（约 3.77 MB）及同页的 SHA-256 文件。双击安装，完成后按 Win + 空格选择“本地拼音”。空格选择首项，数字 1–9 选词，左右方向键移动候选，Esc 取消，Enter 提交原始拼音，Ctrl + Space 切换中英文。升级后重新打开正在输入的应用，以加载新版本。

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

测试覆盖词库、全拼/简拼/容错、整句候选、学习持久化和并发更新、DPI 布局、TSF 生命周期及 COM DLL。另有固定138句、第二组67句和第三组未训练文章/日常文本验证；真实正确率、候选覆盖与逐键耗时见 [输入质量评测](docs/TYPING_EVALUATION.md)。长句和同音词仍可能需要选词修正。TSF 集成测试使用真实 Windows 文档/上下文/编辑会话，键盘注册通过测试适配器隔离；不修改当前输入法注册。因此该测试不等同于逐款应用的安装后兼容性测试。

本版本为未签名的 x64 本地构建。32 位应用尚未支持。Microsoft Store 等部分沉浸式宿主的自绘候选窗仍存在兼容性限制。主机应用行为可能不同；出现无法输入时可按 Win + 空格切回其他输入法并重新打开应用。

## 许可

代码和项目维护的第一方词库采用 MIT 许可，见 LICENSE。独立标准词库来自固定版本的 Rime/AOSP PinyinIME 数据，采用 Apache-2.0；完整声明随安装包提供。上下文模型使用独立原创语料和公有领域原文的短字符统计。分发与来源记录见 `resources/dictionary/sources/lexicon_sources.json` 和 `resources/dictionary/authoring/sources.json`，统计与 SHA-256 保留在元数据内。Python、完整作者语料与词库维护工具均不进入安装包。
