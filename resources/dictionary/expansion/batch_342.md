# 批次 342：自行车与骑行

- 条数：320；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 318 / 2 条；同音权重上限导致 2 条降档。
- 含多音字的 261 个词按字记录在 `batch342_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`0bd407e48d041dc7a03c8486c95d42d1d1a27ab8dc90b547133b44e7d9ba6871`。
- 可打出清单：`probes/batch342_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round23.md`。
