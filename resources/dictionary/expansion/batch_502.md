# 批次 502：临时工作交接与协作聊天

- 条数：388；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 388 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 359 个词按字记录在 `batch502_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`d1e29ae20838190ddf33bbc9f54416a7089e7ac7b1f684e66f9da9f497ef872e`。
- 可打出清单：`probes/batch502_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round47.md`。
