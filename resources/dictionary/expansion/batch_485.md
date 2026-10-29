# 批次 485：日常拍照与相片分享聊天

- 条数：323；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 323 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 297 个词按字记录在 `batch485_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`499d17efa17d723b5d96008f87f6de41ef811ef2ec749b99b779edb78a66fa9f`。
- 可打出清单：`probes/batch485_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round44.md`。
