# 批次 421：约见等候与临近出发

- 条数：432；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 432 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 391 个词按字记录在 `batch421_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`46dbe0b3273d1a69a4d7f91ac05d97c29d4ad6c14bf9e7c5eb33f76848ad579a`。
- 可打出清单：`probes/batch421_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round35.md`。
