# 批次 462：偏好取舍与观看讨论聊天

- 条数：358；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 358 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 332 个词按字记录在 `batch462_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`afc9580b190a6170013ccd79c1cdeccd70eb973470e094fb8b4bedf83df61f43`。
- 可打出清单：`probes/batch462_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round40.md`。
