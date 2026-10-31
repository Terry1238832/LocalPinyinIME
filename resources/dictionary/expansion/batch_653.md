# 批次 653：日常聊天：天气变化、穿衣出门与到家提醒的长句

- 条数：10；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 10 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 10 个词按字记录在 `batch653_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`6e2c706cdccaa731b83198e003b8cc9e0338ac8705ba0e64a8ed5054a8d53ebc`。
- 可打出清单：`probes/batch653_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round70.md`。
