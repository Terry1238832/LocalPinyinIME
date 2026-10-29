# 批次 505：临时工作交接与协作较长说法

- 条数：21；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 21 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 21 个词按字记录在 `batch505_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`a241a5c8dd8d49dea5d5f2575c4d1ddf294e0e48ca6908f0ba299a41ea22ef53`。
- 可打出清单：`probes/batch505_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round47.md`。
