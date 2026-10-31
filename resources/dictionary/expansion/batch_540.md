# 批次 540：意见不同与协商折中聊天

- 条数：446；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 446 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 407 个词按字记录在 `batch540_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`89e1d330ec500ab4709ba96563b1e1ffa27002bdacf060e69bd9e013dda8115f`。
- 可打出清单：`probes/batch540_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round53.md`。
