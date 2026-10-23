# 批次 233：旅游与酒店住宿

- 条数：389
- 来源：项目逐条自拟（first-party authored），未导入第三方词库，未从 `tests/data/` 取词。
- 权重：6000 有 25 条，5500 有 360 条，5000 有 4 条。同全拼不超过已有不低于 5000 的最低档，3 条因此降档。
- 复核：含多音字的 326 个词在 `batch233_pronunciation_review.tsv` 里逐字记录；一名 AI 作者按语义复核，不声称独立人工语言学审校。
- 流程：`curate_lexicon.py` prepare → 写读音复核 → freeze → apply。
- 冻结 TSV SHA-256：`e48269496889503f74587794707dfccb473047d0a2441e85f9ce6c62154e244c`。
- 可打出清单：`probes/batch233_probe.tsv`。
- 原生候选检查和准确率结果见 `docs/lexicon-keep-adding-round9.md`（整轮完成后记录）。
