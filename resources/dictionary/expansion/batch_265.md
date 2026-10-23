# 批次 265：直播带货与电商运营

- 条数：500；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 500 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 334 个词按字记录在 `batch265_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`680c7c613cfe61032858c686ab4539da319f8c9197a4872e9693551bffacebac`。
- 可打出清单：`probes/batch265_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round13.md`。
