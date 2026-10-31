# 批次 640：日常聊天：阅读分享、推荐书文与聊聊读后想法

- 条数：346；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 346 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 329 个词按字记录在 `batch640_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`7524b0e6b04f98a91f1b3049bb4df922b63bc7cba6517e48f8e8ec3662fee6dd`。
- 可打出清单：`probes/batch640_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round69.md`。
