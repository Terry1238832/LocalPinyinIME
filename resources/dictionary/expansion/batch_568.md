# 批次 568：自嘲与小失误的轻松回应

- 条数：300；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 300 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 286 个词按字记录在 `batch568_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`362bc77386287efab127d62dd55def3bbcafafeaa7f3d1343891f4b203432784`。
- 可打出清单：`probes/batch568_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round58.md`。
