# 本地拼音 0.6.0

Windows 10/11 x64 离线中文拼音输入法。输入核心是原生 C++20/Windows TSF，设置程序使用 Electron、React、TypeScript 和 Vite。设置窗口关闭后退出，输入核心继续独立工作。安装包自带桌面运行时，使用时无需联网，也无需另外安装 Node.js、Python或浏览器。

设置采用新的明暗界面，提供跟随系统、候选字号、真实候选预览、Shift切换设置、个人词库增改删查、内置词库查询与学习清除。侧栏和五个页面保持挂载，切换只改变可见内容；搜索和编辑状态保留。启动先读取保存的主题和配置，避免浅色首屏再变深色。全部操作连接原生配置、词库和输入引擎，沿用旧版个人数据。

支持全拼、连续拼音与整句候选、简拼、补全、单次拼写容错、中文标点、鼠标选词和本地学习。内置词库83,062条；常用个人长词频率5000及以上、完整拼音匹配时保留直接候选。输入质量与0.5.1一致，长句和同音词仍可能需要选词。

从 [GitHub Release v0.6.0](https://github.com/Terry1238832/LocalPinyinIME/releases/tag/v0.6.0) 下载 [Windows x64安装包](https://github.com/Terry1238832/LocalPinyinIME/releases/download/v0.6.0/LocalPinyinIME-0.6.0-x64.exe)（100.30 MiB）及同页SHA-256文件。Electron运行时使安装内容约326.95 MiB，已只保留中文和英文语言资源。双击安装后按Win＋空格选择“本地拼音”；升级后重新打开输入应用。空格确认首项，数字1–9选词，Esc取消，Enter提交原始拼音，Ctrl＋Space切换中英文。

![本地拼音设置](docs/images/interface.png)

## 构建

需要Visual Studio C++工具和Windows SDK、CMake/Ninja、Python 3、Node.js 22.12或更高版本及Inno Setup 6。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build.ps1
```

脚本构建并验证原生组件、使用npm锁文件安装作者依赖、编译Vite与Electron、打包原生桥接和词库，最后生成dist中的安装包及校验文件。首次需要联网下载npm依赖和Electron；已有缓存的Electron压缩包按官方校验值验证后可离线重用。开发设置程序可在desktop中运行npm ci及npm run dev，原生桥接须先构建。

源码分为src/engine（输入与学习）、src/ime（TSF和原生候选窗）、src/settings（共用配置与词库目录）、src/tools/settings_bridge.cpp（本地JSON接口）、desktop/src（React页面）及desktop/electron（主进程与隔离preload）。旧设置窗口和旧截图程序已移除。

## 数据与验证

个人设置、词库和学习沿用%LOCALAPPDATA%\LocalPinyinIME；桌面运行时缓存位于其desktop子目录。学习最多保存2,048条记录；清除学习保留个人词库，卸载保留个人数据。预览只查询，不学习或上传输入。

32项原生回归通过（含9个跨进程桥接场景），真实交付ASAR通过12个完整桌面流程、80次侧栏切换和34张截图检查。267句输入回归与0.5.1相同。详情见 [验证记录](docs/verification.json)、[输入质量](docs/TYPING_EVALUATION.md)及 [兼容范围](docs/COMPATIBILITY.md)。未完成干净机器完整安装卸载和全部应用的安装后验收。

## 许可

项目代码与原创词库采用MIT；标准词库为Apache-2.0，字符模型使用原创和公有领域语料。Electron/React采用MIT；Chromium及运行时组件声明随安装包保留在LICENSE.electron.txt、LICENSES.chromium.html和THIRD_PARTY_NOTICES.txt。作者工具、训练原文和node_modules不进入安装包。
