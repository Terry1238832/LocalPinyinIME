# 批次 608：追剧听歌、观影和演出后的聊天

- 条数：352；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 352 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 333 个词按字记录在 `batch608_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`f663627ba829944d6e2511e11ffb508e2e939a8100efb7db75f3cfe3a3af32b5`。
- 可打出清单：`probes/batch608_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round64.md`。
