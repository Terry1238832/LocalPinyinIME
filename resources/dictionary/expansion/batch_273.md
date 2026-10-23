# 批次 273：物流仓储

- 条数：500；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 500 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 340 个词按字记录在 `batch273_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`83a39b6eb51b9eb712e45125711176f750b22b402f2d03f484d9d9e2d0dfa5f4`。
- 可打出清单：`probes/batch273_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round14.md`。
