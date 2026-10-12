运行 scripts/Build.ps1。脚本自动发现 MSVC、CMake/Ninja、Python 3 和 Inno Setup 6，完成 Release 构建、回归验证和安装包生成。详见源码 README.md。

普通构建使用已校验的词库和上下文模型，不联网且不需要 pypinyin。词库维护和模型重新生成是单独的作者流程；来源、固定版本、许可证与 SHA-256 保留在 resources/dictionary 的元数据中。
