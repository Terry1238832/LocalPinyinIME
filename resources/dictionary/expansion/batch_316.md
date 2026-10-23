# 批次 316：二手交易与闲置较长说法

- 条数：19；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 19 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 18 个词按字记录在 `batch316_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`155169b1fdde9e3d99177bb1fd1a51a19ab80c54d1f7a81d8c51a6c30ecbc403`。
- 可打出清单：`probes/batch316_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round19.md`。
