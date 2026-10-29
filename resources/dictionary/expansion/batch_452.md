# 批次 452：换洗晾晒与穿衣聊天

- 条数：351；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 351 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 329 个词按字记录在 `batch452_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`ba7609e0b5720c23b6c21fba00e96b35405ac38e72c9962063bf2ead46fa286c`。
- 可打出清单：`probes/batch452_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round39.md`。
