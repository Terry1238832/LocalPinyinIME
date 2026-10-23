# 批次 286：环保与能源

- 条数：500；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 499 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 365 个词按字记录在 `batch286_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`d9150c47c3dd079bf668f116d586c24c768bdc8ef8fb82f7442f62c1032ab982`。
- 可打出清单：`probes/batch286_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round16.md`。
