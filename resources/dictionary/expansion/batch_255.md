# 批次 255：户外露营徒步

- 条数：456；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 454 / 2 条；同音权重上限导致 2 条降档。
- 含多音字的 351 个词按字记录在 `batch255_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`1296b5f4a6c3de13fc47e7e6f6dffa91434309047af91e5a3b6db99a380fec31`。
- 可打出清单：`probes/batch255_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round12.md`。
