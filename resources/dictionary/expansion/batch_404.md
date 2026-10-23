# 批次 404：婴儿喂奶拍嗝与奶具使用

- 条数：119；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 119 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 87 个词按字记录在 `batch404_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`4e514d10dfb174cce3b219ead09709cd454ba3290d95f974f73ec00249ef7981`。
- 可打出清单：`probes/batch404_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round31.md`。
