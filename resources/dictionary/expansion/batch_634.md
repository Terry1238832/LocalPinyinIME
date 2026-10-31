# 批次 634：日常聊天替人转达留言与回电确认的长句

- 条数：15；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 15 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 15 个词按字记录在 `batch634_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`86aacb071dae4253bc5a157c2c8c2116f2dd586a0b9dfa487676e6839ea79b16`。
- 可打出清单：`probes/batch634_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round68.md`。
