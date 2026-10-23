# 批次 231：手机与应用操作

- 条数：347
- 来源：项目逐条自拟（first-party authored），未导入第三方词库，未从 `tests/data/` 取词。
- 权重：6000 有 2 条，5500 有 345 条，5000 有 0 条。同全拼不超过已有不低于 5000 的最低档，0 条因此降档。
- 复核：含多音字的 265 个词在 `batch231_pronunciation_review.tsv` 里逐字记录；一名 AI 作者按语义复核，不声称独立人工语言学审校。
- 流程：`curate_lexicon.py` prepare → 写读音复核 → freeze → apply。
- 冻结 TSV SHA-256：`1b1939c548c19261179edda2a09fcb2b57a5916b33b00017463ba01b472e17e9`。
- 可打出清单：`probes/batch231_probe.tsv`。
- 原生候选检查和准确率结果见 `docs/lexicon-keep-adding-round9.md`（整轮完成后记录）。

## 第33轮自查勘误

弹窗的弹按“弹出窗口”读 tan，修正全拼与逐字复核、当前批次哈希和探针；词面与权重不变。原始追加回执的冻结哈希及 canonical 前后哈希保留历史含义，当前修订见 `round33_corrections.json` 和 `.json` 的 `correction_history`。重新运行原生候选检查，结果见第33轮记录。
