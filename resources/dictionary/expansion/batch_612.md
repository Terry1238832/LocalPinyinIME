# 批次 612：约饭选店、口味偏好与点单聊天

- 条数：438；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 438 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 392 个词按字记录在 `batch612_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`6a3a0c747e11bd314e6042b071e5195920e2d5c06d76a2a05bd2cbdfd420e1c9`。
- 可打出清单：`probes/batch612_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round65.md`。
