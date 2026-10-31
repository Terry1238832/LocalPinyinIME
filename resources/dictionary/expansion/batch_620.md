# 批次 620：朋友之间共同开销、代付与结算聊天

- 条数：336；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 336 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 309 个词按字记录在 `batch620_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`d7e87b56bf904fc04d029fd02d6babaaa526f777595e304269b1633272007837`。
- 可打出清单：`probes/batch620_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round66.md`。
