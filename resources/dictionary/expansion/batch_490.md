# 批次 490：备菜下厨与火候提醒聊天

- 条数：323；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 323 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 290 个词按字记录在 `batch490_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`d019dacd60ba546fe08df178ff03dc0b433834ce7bedafc2067bcaa708809a3c`。
- 可打出清单：`probes/batch490_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round45.md`。
