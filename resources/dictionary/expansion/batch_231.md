# 批次 231：手机与应用操作

- 条数：347
- 来源：项目逐条自拟（first-party authored），未导入第三方词库，未从 `tests/data/` 取词。
- 权重：6000 有 2 条，5500 有 345 条，5000 有 0 条。同全拼不超过已有不低于 5000 的最低档，0 条因此降档。
- 复核：含多音字的 265 个词在 `batch231_pronunciation_review.tsv` 里逐字记录；一名 AI 作者按语义复核，不声称独立人工语言学审校。
- 流程：`curate_lexicon.py` prepare → 写读音复核 → freeze → apply。
- 冻结 TSV SHA-256：`1da46df67f841a4e080a1dbf37ce080a9b7c92c25c5f2136df45e5d4611ccad1`。
- 可打出清单：`probes/batch231_probe.tsv`。
- 原生候选检查和准确率结果见 `docs/lexicon-keep-adding-round9.md`（整轮完成后记录）。
