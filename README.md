# 本地拼音 0.7.4

Windows 10/11 x64离线中文拼音输入法。输入核心是原生C++20/Windows TSF，设置使用Electron、React、TypeScript和Vite。关闭设置后桌面程序退出，输入核心继续独立工作。安装包自带运行时，使用时无需联网或另外安装Node.js、Python。

本轮接入GitHub新增的41,526行词库，并新增7,041条学校职场、生活服务、旅行运动、家电维修与家庭照护词语，内置词库共142,220条。少数同音词仍可能超出前九候选；详见来源审计和逐词实测。完整长词匹配时保留可选候选，避免被拆开的同音候选挤出。

候选栏只显示一行，按字号和窗口可用宽度分页，左右箭头或窗口箭头切换。每页数字从1重新编号，空格接受当前页选中的候选；长词省略显示但完整上屏。设置页的拼音与联想预览也使用单行分页。

前文联想：继续打拼音时参考当前光标前的文字排序；词语上屏后，用346条原创联想关系、词库短语和已有选词学习补全后续词。按Tab接受联想、Esc关闭，空格和数字保留普通编辑行为。“输入”设置可关闭联想，也能在试用区查看真实结果。使用说明见[前文联想](docs/CONTEXT_PREDICTION.md)。

设置保留原蓝色PI品牌和侧栏图标、明暗主题、七档字号、真实候选预览、Shift切换、个人词库增改删查、内置词库查询与学习清除。五页保持挂载，切换保留输入状态；启动先读取保存主题和配置。

从[GitHub Release v0.7.4](https://github.com/Terry1238832/LocalPinyinIME/releases/tag/v0.7.4)下载[Windows x64安装包](https://github.com/Terry1238832/LocalPinyinIME/releases/download/v0.7.4/LocalPinyinIME-0.7.4-x64.exe)（100.83 MiB）及同页SHA-256文件。安装内容约328.68 MiB，仅保留中文与英文运行时语言资源。双击安装后按Win＋空格选择“本地拼音”；升级后重新打开输入应用。

普通拼音候选使用空格确认当前选项、数字1–9选择当前页对应词、左右箭头翻页、Esc取消、Enter提交原始拼音；Ctrl＋Space切换中英文。支持全拼、连续拼音与整句候选、简拼、补全、单次拼写容错、中文标点、鼠标选词和本地学习。长句和同音词仍可能需要选词。

![原生单行候选](docs/images/candidate-single-line.png)

## 构建

需要Visual Studio C++工具和Windows SDK、CMake/Ninja、Python 3、Node.js 22.12或更高版本及Inno Setup 6。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build.ps1
```

脚本构建并验证原生组件、按npm锁文件安装作者依赖、编译Vite与Electron、打包桥接和五份运行数据，生成dist中的安装包及校验文件。首次需要联网下载依赖和Electron；官方校验后的缓存压缩包可重用。desktop中运行npm ci及npm run dev开发设置页，原生桥接须先构建。数据生成统一UTF-8/LF，当前SHA与Git检出一致。

## 数据与验证

个人设置、词库和学习沿用%LOCALAPPDATA%\LocalPinyinIME；Electron缓存在desktop子目录。学习最多保存2,048条记录；清除学习保留个人词库，卸载保留个人数据。前文查询和试用在本机完成，接受联想不新增学习记录，普通拼音选词沿用原有学习。

34项原生回归通过（含10个跨进程桥接场景），实际交付ASAR通过15个桌面流程、80次侧栏切换及41张截图检查。267句公开回归首选152/267、前九覆盖206/267，结果见[输入质量记录](docs/lexicon-017-quality.json)；新增原创词逐项原生查询与上游名称审计分开记录。完整验证见[verification.json](docs/verification.json)和[兼容范围](docs/COMPATIBILITY.md)。未完成干净机器完整安装卸载和全部应用的安装后验收。

## 许可

项目代码、原创词库与联想关系采用MIT；标准词库为Apache-2.0，字符模型使用原创和公有领域语料。上游页面标题整理的名称索引有独立来源记录及原站条款，不统一声称MIT授权；参见[来源审计](docs/remote-lexicon-audit-074.md)。Electron/React采用MIT；运行时组件声明保留在LICENSE.electron.txt、LICENSES.chromium.html和THIRD_PARTY_NOTICES.txt。作者工具、训练原文与node_modules不进入安装包。
