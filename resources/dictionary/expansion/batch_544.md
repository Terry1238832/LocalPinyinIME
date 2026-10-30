# 批次 544：网络联系不畅与消息延迟聊天

- 条数：416；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 416 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 381 个词按字记录在 `batch544_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`bd91808fd12440d534e1b3de3154c4ea82decb44a4acde6c4cda144a423146ae`。
- 可打出清单：`probes/batch544_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round54.md`。
