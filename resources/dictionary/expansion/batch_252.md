# 批次 252：婚恋与婚礼较长说法

- 条数：20；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 20 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 19 个词按字记录在 `batch252_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`0e70e01fb0188c93fb58ae10c6234925f0ef2a5cdd05a4bad71fe3c2f5b429df`。
- 可打出清单：`probes/batch252_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round11.md`。
