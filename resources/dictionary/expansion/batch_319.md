# 批次 319：智能家居

- 条数：404；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 404 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 299 个词按字记录在 `batch319_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`dd06ac550ec8bf310b7e6c58a36ef0932d9eb803a31a02962db64d704fb6edf8`。
- 可打出清单：`probes/batch319_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round20.md`。
