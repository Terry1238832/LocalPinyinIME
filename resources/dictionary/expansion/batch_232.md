# 批次 232：求职招聘与劳动

- 条数：394
- 来源：项目逐条自拟（first-party authored），未导入第三方词库，未从 `tests/data/` 取词。
- 权重：6000 有 7 条，5500 有 386 条，5000 有 1 条。同全拼不超过已有不低于 5000 的最低档，0 条因此降档。
- 复核：含多音字的 293 个词在 `batch232_pronunciation_review.tsv` 里逐字记录；一名 AI 作者按语义复核，不声称独立人工语言学审校。
- 流程：`curate_lexicon.py` prepare → 写读音复核 → freeze → apply。
- 冻结 TSV SHA-256：`8da3de81f29e308707fe9125cd63be646fe34530700c57342709a22535d24f41`。
- 可打出清单：`probes/batch232_probe.tsv`。
- 原生候选检查和准确率结果见 `docs/lexicon-keep-adding-round9.md`（整轮完成后记录）。
