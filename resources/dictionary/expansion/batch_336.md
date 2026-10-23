# 批次 336：新闻阅读与信息辨识

- 条数：353；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 353 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 252 个词按字记录在 `batch336_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`3a0511ec7ea413b0b686f7fc663842c6c6aed0d05851326e43480322dcc5f21f`。
- 可打出清单：`probes/batch336_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round22.md`。
