# 批次 547：网络联系不畅与消息延迟较长说法

- 条数：10；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 10 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 10 个词按字记录在 `batch547_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`c447fe6375449a90a0b89057f9e0a0d08eb5011cffc59c69cb0ff3fae24f9e00`。
- 可打出清单：`probes/batch547_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round54.md`。
