# 批次 498：逛店试穿与挑选商量聊天

- 条数：327；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 327 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 305 个词按字记录在 `batch498_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`addb3fba273073999bf3343852f18a5418b723cc7c1f8508fce05d707a47b1ad`。
- 可打出清单：`probes/batch498_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round46.md`。
