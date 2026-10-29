# 批次 445：数量分量与多少问答聊天

- 条数：460；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 460 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 387 个词按字记录在 `batch445_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`8b1fbd7781047438c53ff6d7805f59c0b089add1fe78c2f84b8feb6eb1275aa0`。
- 可打出清单：`probes/batch445_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round38.md`。
