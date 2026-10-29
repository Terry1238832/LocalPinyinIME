# 批次 460：饭菜口味与下厨搭手聊天

- 条数：318；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 318 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 295 个词按字记录在 `batch460_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`14edbdb210dccdb3c4bfbedb06d0bddff85b6833575635a52c4c4c4e77661a79`。
- 可打出清单：`probes/batch460_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round40.md`。
