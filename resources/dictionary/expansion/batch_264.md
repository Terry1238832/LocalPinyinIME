# 批次 264：网络小说用语

- 条数：500；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 499 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 336 个词按字记录在 `batch264_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`05d2c2dc7bad58a6f25d70090cf9a227d96cbfa485ea62e42c14b452fb128f9b`。
- 可打出清单：`probes/batch264_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round13.md`。
