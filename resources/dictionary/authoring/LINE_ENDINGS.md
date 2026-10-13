# 数据文件的换行与哈希

资源 TSV、作者文本、批次和来源 JSON、冻结测试 TSV 统一使用 UTF-8 与 LF 换行，由仓库 `.gitattributes` 固定。Python 数据生成器显式使用 `write_text(..., encoding="utf-8", newline="\n")`，以确保 Windows 生成文件与 Git 干净检出的字节一致。

当前资源 metadata 的 `sha256`、来源文件的文本校验值、批次输出的 `sha256` / `frozen_batch_sha256`、冻结测试文件的校验值均指向当前 LF 文件。2026-10-12 的换行迁移只去除 CRLF 中的 CR，没有改变词条、拼音、权重、作者正文或测试答案。

批次 `dedup_baseline_sha256`、`canonical_before_sha256` 和已被后续合并覆盖的 `canonical_after_sha256` 是编写或合并当时的历史字节快照，按原值保存。它们不是当前词库文件的校验值，可能采用当时的 CRLF。`original_crlf_*` 字段同样保留原始字节证据。第三方上游下载摘要和从中文正文计算的独立摘要也保留原值。

合并后的批次已冻结。重新针对包含该批次的当前词库运行作者脚本会过滤掉已经收录的词条，不能覆盖原始批次存档。需要复现批次时，应使用对应合并前的词库快照；只验证当前发行数据时，应使用当前资源 metadata 与 `tools/validate_*.py`。
