# 批次 400：祝福语与日常问候补漏

- 条数：140；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 140 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 119 个词按字记录在 `batch400_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`6996902b98edde5896224a81d82521750f07150f9db8b1b9f09c39c0ae920e61`。
- 可打出清单：`probes/batch400_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round30.md`。
