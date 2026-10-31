# 批次 583：群聊活动接龙、确认人数与临时调整长句

- 条数：24；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 24 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 24 个词按字记录在 `batch583_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`2e4f6d4ba1930de137ea98f329c61131a557e2d149fcee62bb6c5194ec530f1d`。
- 可打出清单：`probes/batch583_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round60.md`。
