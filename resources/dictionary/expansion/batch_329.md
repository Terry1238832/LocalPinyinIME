# 批次 329：通信套餐与号码业务

- 条数：384；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 383 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 316 个词按字记录在 `batch329_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`839b9f7e879806e14d7c2e37e184293ffdbd09e738dbfca5c8cfd257a20f7b81`。
- 可打出清单：`probes/batch329_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round21.md`。
