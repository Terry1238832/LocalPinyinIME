# 批次 431：解释确认与商量说法

- 条数：317；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 317 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 303 个词按字记录在 `batch431_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`e2463e8e5941ef6ece0aca0c4363f4b390af2929fce7f5f9aaa93dc1e078ab6f`。
- 可打出清单：`probes/batch431_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round36.md`。
