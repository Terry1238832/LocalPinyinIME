# 批次 305：考研与考公

- 条数：484；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 484 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 345 个词按字记录在 `batch305_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`958b876f4d02e474cef8e29f3b7a944fe0c18b2d6371405f715d330e31c895f2`。
- 可打出清单：`probes/batch305_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round18.md`。
