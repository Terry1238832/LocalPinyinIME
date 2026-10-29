# 批次 503：现场排队等候与问询聊天

- 条数：340；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 340 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 308 个词按字记录在 `batch503_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`a066487fa2ec79365d2d46b050155d81ef4807a3cf5dfc2a59dfadb2d510aa70`。
- 可打出清单：`probes/batch503_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round47.md`。
