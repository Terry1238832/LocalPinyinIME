# 批次 446：物件指认与位置远近聊天

- 条数：367；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 367 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 312 个词按字记录在 `batch446_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`22073a19dd1c5e72e0559b3299281bc2406aff81f10cd6b4f6dbda309ed816ef`。
- 可打出清单：`probes/batch446_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round38.md`。
