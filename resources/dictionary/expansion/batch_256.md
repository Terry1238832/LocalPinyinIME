# 批次 256：钓鱼

- 条数：500；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 494 / 6 条；同音权重上限导致 6 条降档。
- 含多音字的 302 个词按字记录在 `batch256_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`ac81a84caad76911200d79687df6f7e839046ffe7027187a1b62b7bc36b03192`。
- 可打出清单：`probes/batch256_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round12.md`。
