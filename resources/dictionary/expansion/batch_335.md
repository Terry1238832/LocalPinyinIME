# 批次 335：商超理货与零售门店

- 条数：364；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 363 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 259 个词按字记录在 `batch335_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`833c1c9beb8d596276804d681f43e803a8a2a1f05ea17e7c2fae4df993c09966`。
- 可打出清单：`probes/batch335_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round22.md`。
