# 批次 570：意外发现、核实与好奇追问

- 条数：310；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 310 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 286 个词按字记录在 `batch570_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`a73a2b14e1cab2d4073abb3e56c5845d950aa5b7e4b5cbf9f0c2842118cb4f3e`。
- 可打出清单：`probes/batch570_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round58.md`。
