# 批次 585：群聊征求意见、发起投票与公布结果长句

- 条数：19；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 19 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 19 个词按字记录在 `batch585_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`0abe8b8ea3fc5df7120548435287b24b33c85d071e2c113f2e0de72e04ef6da3`。
- 可打出清单：`probes/batch585_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round60.md`。
