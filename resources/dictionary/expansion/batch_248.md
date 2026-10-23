# 批次 248：婚恋与婚礼

- 条数：441；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 441 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 316 个词按字记录在 `batch248_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`fbdf8c8e10b920044f700c1ad9bde6170134995e6694686d4fa0a15d6a0e651a`。
- 可打出清单：`probes/batch248_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round11.md`。
