# 批次 406：餐饮后厨备料与出餐协作

- 条数：117；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 117 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 96 个词按字记录在 `batch406_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`83e9789f90b4c8a6ec151d16c37b275b8fe8393f7709fe2fe50a0cadc7cf868f`。
- 可打出清单：`probes/batch406_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round31.md`。
