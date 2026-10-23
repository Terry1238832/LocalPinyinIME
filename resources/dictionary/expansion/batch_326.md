# 批次 326：语文语法与修辞

- 条数：380；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 378 / 2 条；同音权重上限导致 2 条降档。
- 含多音字的 240 个词按字记录在 `batch326_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`9c63094d70489ed8333231ca30a84dddcbe4a8ec676bf5d63a7c6bd1f9f1e7c7`。
- 可打出清单：`probes/batch326_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round21.md`。
