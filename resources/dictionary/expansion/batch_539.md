# 批次 539：接收反馈与说明调整聊天

- 条数：403；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 403 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 359 个词按字记录在 `batch539_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`b021ab1e6c970bacb441a3d3e1484430a5726f616afb11f425b0bb511398dc77`。
- 可打出清单：`probes/batch539_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round53.md`。
