# 批次 352：出版印刷与图书制作

- 条数：365；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 363 / 2 条；同音权重上限导致 2 条降档。
- 含多音字的 221 个词按字记录在 `batch352_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`7bd28748d93c16f93142b6134190ad72f609e881806b4129ff5ad1f8a5abbeb7`。
- 可打出清单：`probes/batch352_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round24.md`。
