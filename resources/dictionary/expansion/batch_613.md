# 批次 613：分享照片与日常动态的互动评论

- 条数：430；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 430 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 380 个词按字记录在 `batch613_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`3c2022f56e3268d479d6a04c9feed3e6d212bce3a83bc1af248403a1a106fbc8`。
- 可打出清单：`probes/batch613_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round65.md`。
