# 批次 594：语音留言的录制、发送与收听

- 条数：378；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 378 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 338 个词按字记录在 `batch594_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`90784413875ad00f22ef60171c6046d38129e7e1e1ce7ac0eeb132ab79bdc1c3`。
- 可打出清单：`probes/batch594_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round62.md`。
