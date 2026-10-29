# 批次 495：看书学习与记笔记交流较长说法

- 条数：20；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 20 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 20 个词按字记录在 `batch495_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`e24706b790d8daeac647a28a1c5661c2c761744903d5d7cfe41110b2b1bb58ea`。
- 可打出清单：`probes/batch495_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round45.md`。
