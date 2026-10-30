# 批次 532：夸赞与表达认可聊天

- 条数：374；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 374 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 346 个词按字记录在 `batch532_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`8c92595e40f86f7222f2d545dc90970748b6b8ac849c1caaa6d1bb069a2cc9aa`。
- 可打出清单：`probes/batch532_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round52.md`。
