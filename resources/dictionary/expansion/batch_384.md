# 批次 384：门窗窗帘与遮阳用品

- 条数：324；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 323 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 299 个词按字记录在 `batch384_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`9b5732e18b4996eee837c9addcd9fe9ef3aacd861a9d77453015dcd88ca796fb`。
- 可打出清单：`probes/batch384_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round28.md`。
