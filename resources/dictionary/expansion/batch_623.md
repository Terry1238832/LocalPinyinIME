# 批次 623：朋友之间共同开销结算的长句

- 条数：15；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 15 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 15 个词按字记录在 `batch623_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`26cf21d55ad47eef31c46bd7913df98bdffa5e0fd199d11936f16c07dcc1a96f`。
- 可打出清单：`probes/batch623_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round66.md`。
