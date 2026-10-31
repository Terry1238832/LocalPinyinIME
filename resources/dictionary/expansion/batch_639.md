# 批次 639：日常聊天：追剧观影、交流剧情与即时观感

- 条数：327；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 327 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 308 个词按字记录在 `batch639_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`40b42d5c5798713cd82e8f08f81c3743ad370c4c09859fcb3030575ca00a68db`。
- 可打出清单：`probes/batch639_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round69.md`。
