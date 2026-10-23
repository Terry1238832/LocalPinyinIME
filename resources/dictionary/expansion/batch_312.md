# 批次 312：二手交易与闲置

- 条数：405；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 405 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 276 个词按字记录在 `batch312_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`e0d02fa56b353cabc2e4dd3156972f0b852675990026badbedc36e8789deeb46`。
- 可打出清单：`probes/batch312_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round19.md`。
