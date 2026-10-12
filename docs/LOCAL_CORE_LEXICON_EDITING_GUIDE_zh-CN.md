内置词库位于 resources/dictionary/local_core_zh_pinyin.tsv。每行依次为拼音、中文词语、排序权重，以制表符分隔。拼音应小写，词条不得重复。个人常用词建议通过设置里的“我的词库”编辑，无需修改安装目录。

更新内置词库后请同步元数据中的词条数、SHA-256 和分类统计，并运行 scripts/Build.ps1 的词库校验。新批次和人工读音修复计划保留在 resources/dictionary/expansion 中，用于去重、来源复核和重新生成；这些作者文件不进入安装包。
