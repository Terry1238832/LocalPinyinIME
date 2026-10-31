# 批次 563：漏看消息与回复晚了后的道歉聊天

- 条数：380；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 380 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 344 个词按字记录在 `batch563_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`1222a0c47021cbc999e35c1276bfc15119aaa3be3137950e894e1b88ce4031c0`。
- 可打出清单：`probes/batch563_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round57.md`。
