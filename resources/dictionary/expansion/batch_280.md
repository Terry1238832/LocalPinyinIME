# 批次 280：亲属称谓与家庭关系

- 条数：400；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 399 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 390 个词按字记录在 `batch280_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`9d6ab4b368f00181142c62d874f15936e3a0931d24a2e97c00ed0fbb7962efaf`。
- 可打出清单：`probes/batch280_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round15.md`。
