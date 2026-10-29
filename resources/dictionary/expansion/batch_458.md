# 批次 458：电话听说与屏幕操作较长说法

- 条数：20；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 20 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 20 个词按字记录在 `batch458_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`41e4fb44a0ed85eed11654f6b1296e5926f253da034ba5036ea2a7ad4f45483a`。
- 可打出清单：`probes/batch458_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round39.md`。
