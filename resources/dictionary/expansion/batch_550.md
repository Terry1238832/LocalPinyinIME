# 批次 550：亲友赠礼与收下心意聊天

- 条数：344；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 344 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 318 个词按字记录在 `batch550_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`8a6a6e658a932ee62da14201409732ab94a7a4fece60c854611c7e6668f58c44`。
- 可打出清单：`probes/batch550_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round55.md`。
