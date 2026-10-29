# 批次 510：拼图拼装与手作协作聊天

- 条数：371；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 371 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 342 个词按字记录在 `batch510_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`8e04e92cedb55ced9d6965cae279d85ab45ad9cac5b0147c9d16a5dc065ef569`。
- 可打出清单：`probes/batch510_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round48.md`。
