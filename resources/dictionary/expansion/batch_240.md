# 批次 240：中学物理化学生物课本用语

- 条数：488；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 486 / 2 条；同音权重上限导致 2 条降档。
- 含多音字的 383 个词按字记录在 `batch240_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`2594861fc706bfbe2d3b49f8386b062d615668c25e60cd814051df30cfc2aa05`。
- 可打出清单：`probes/batch240_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round10.md`。
