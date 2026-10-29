# 批次 449：数量分量与多少问答较长说法

- 条数：20；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 20 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 20 个词按字记录在 `batch449_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`a1e891b699e92d603d48f58fbd6dfba4dec90d8cee70baf222f01a2357e7cb66`。
- 可打出清单：`probes/batch449_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round38.md`。
