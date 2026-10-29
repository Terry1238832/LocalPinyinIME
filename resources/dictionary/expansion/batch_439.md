# 批次 439：小失误与事后补救聊天

- 条数：302；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 302 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 287 个词按字记录在 `batch439_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`965fa9d34b3d4741894518c7a2dcf77727ab05baabcbc10ffd8e1c7905a65604`。
- 可打出清单：`probes/batch439_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round37.md`。
