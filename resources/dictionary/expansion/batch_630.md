# 批次 630：日常聊天：转达消息、留言与确认回话

- 条数：308；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 308 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 253 个词按字记录在 `batch630_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`b66bd42a625eb0c2eeb7701ad786a7c51f569166db04a8737dd5b65b1c57e0e3`。
- 可打出清单：`probes/batch630_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round68.md`。
