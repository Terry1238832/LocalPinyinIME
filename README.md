# LocalPinyinIME

LocalPinyinIME 是一个 Windows 原生 TSF 离线拼音输入法实验项目。它使用 C++、Win32、COM 和 Text Services Framework 实现，目标是在不依赖网络服务的前提下验证一个小型中文拼音输入法的核心流程。

当前状态：`v0.2.0-dev`

平台：Windows x64

构建工具：CMake + Visual Studio 2026 / MSVC

## 功能现状

当前已经实现并验证到源码和本地 Release 包层面的能力：

- TSF 原生输入法框架；
- 离线拼音候选；
- 内置人工维护 TSV 词典；
- 连续拼音分词；
- 数字选词；
- Esc 取消 composition；
- Ctrl + Space 英文直输；
- 本地学习排序；
- Release x64 构建、单元测试和 ZIP 解包 Smoke 检查。

## 当前限制

- 当前是未签名开发版，不是正式发布版本；
- 当前仅面向 Windows x64；
- 尚未承诺兼容 32 位软件；
- 词典仍是小型 MVP 词典；
- 尚未保证所有第三方编辑器、浏览器、IDE 或富文本控件兼容；
- 安装、启用和卸载流程应先在可回滚测试环境中验证。

## 源码构建

在 Visual Studio 2026 Developer PowerShell 中运行：

```powershell
cmake -S . -B build-release-x64 -A x64
cmake --build build-release-x64 --config Release --parallel 1
```

## 单元测试

```powershell
ctest --test-dir build-release-x64 -C Release --output-on-failure
```

## DictionarySmoke

构建后可以直接检查包内词典是否能被工具加载：

```powershell
.\build-release-x64\bin\Release\LocalPinyinImeDictionarySmoke.exe .\resources\dictionary\core_zh_pinyin.tsv
```

Release ZIP 会额外使用 `scripts\Test-ReleasePackage.ps1` 解包验证词典路径、哈希、有效词条数和连续拼音 Smoke 样例。

## Release 包

版本信息集中在：

```text
cmake/LocalPinyinReleaseVersion.cmake
```

生成可重复 ZIP 包：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\Build-Release.ps1" -BuildDir "build-release-x64"
```

输出目录：

```text
dist\
```

`dist` 是生成物目录，不应提交到源码仓库。

## 词典资源

核心词典位于：

```text
resources/dictionary/core_zh_pinyin.tsv
```

该 TSV 是人工维护的项目资源，应随源码提交。构建产物、运行期学习数据、日志、数据库和本地测试输出不应提交。

## 安全与隐私

当前实现目标是离线输入法：

- 不需要网络请求；
- 不注册开机启动项、服务或计划任务；
- 不使用全局键盘 Hook；
- 不使用 SendInput 模拟输入；
- 不读取剪贴板；
- 不修改默认输入法；
- 不修改 Preload、Substitutes 或 Keyboard Layout；
- 本地学习和用户输入偏好属于运行期用户数据，不应上传到源码仓库。

## 许可证

仓库当前存在 `LICENSE` 文件。上传前请由项目作者确认最终许可证选择；如果作者尚未确认，请将许可证视为待确定，未经明确授权不应复制、修改或再分发。
