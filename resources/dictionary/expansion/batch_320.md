# 批次 320：人工智能与大模型日常说法

- 条数：385；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 384 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 300 个词按字记录在 `batch320_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`cc4a1a8bc24cd545110e67802a8c7bdf7eaf296a65a81ceb511017e5ff902fd7`。
- 可打出清单：`probes/batch320_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round20.md`。
