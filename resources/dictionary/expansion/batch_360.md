# 批次 360：儿童玩具与益智玩具

- 条数：321；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 320 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 235 个词按字记录在 `batch360_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`4bd5ccded44dd0b9f6d856df3c2671a06bd1f86719d0162414f2ec4da2e834d9`。
- 可打出清单：`probes/batch360_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round25.md`。
