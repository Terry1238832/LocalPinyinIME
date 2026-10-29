# 批次 428：吃饭点单与餐桌聊天

- 条数：339；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 339 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 321 个词按字记录在 `batch428_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`d5e36f82ca3e9053f36dafad980fe699ca0787241ecdc5b5125f6328fc632e09`。
- 可打出清单：`probes/batch428_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round36.md`。
