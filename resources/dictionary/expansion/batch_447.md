# 批次 447：接话补充与句尾追问聊天

- 条数：401；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 401 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 389 个词按字记录在 `batch447_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`83622844a45876251b8269c6e30179ba0016d7d5cf7df7463e73f610eca80a23`。
- 可打出清单：`probes/batch447_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round38.md`。
