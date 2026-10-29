# 批次 420：聊天联络与消息往来

- 条数：431；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 431 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 375 个词按字记录在 `batch420_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`d2980591ea4662b1693ca3482d9a6df0c6d5c6504f4d0a95121088ed6834218f`。
- 可打出清单：`probes/batch420_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round35.md`。
