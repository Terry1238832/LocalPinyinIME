# 批次 409：餐饮后厨备料与出餐协作较长说法

- 条数：10；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 10 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 10 个词按字记录在 `batch409_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`d1502723c73b0f5367ba8925355e0a8a7578ce56ed6881cf6dff95ca9df93349`。
- 可打出清单：`probes/batch409_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round31.md`。
