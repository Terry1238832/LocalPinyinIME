# 批次 444：时间紧慢与先后安排聊天

- 条数：350；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 350 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 314 个词按字记录在 `batch444_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`6958ca2707680b4f4fc08cca7725701bd098c479334f1e55894dead50817b29c`。
- 可打出清单：`probes/batch444_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round38.md`。
