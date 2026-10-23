# 批次 311：驾考与交通违章

- 条数：471；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 470 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 339 个词按字记录在 `batch311_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`07dbe86b5882860d428801260d4a5ca6d0677331a5574ba4a84bfd6b31419c5a`。
- 可打出清单：`probes/batch311_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round19.md`。
