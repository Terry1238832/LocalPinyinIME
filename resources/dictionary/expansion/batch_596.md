# 批次 596：聊天引用回复、接话与澄清上下文

- 条数：422；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 422 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 387 个词按字记录在 `batch596_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`f2d65894954543a5167384f18e8c63a1abb90364df2fa16be3ca94fc88785b52`。
- 可打出清单：`probes/batch596_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round62.md`。
