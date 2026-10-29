# 批次 486：酒店到店与住店沟通聊天

- 条数：313；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 312 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 282 个词按字记录在 `batch486_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`d11a977d03734fc155c00b3defc42b2ec77f539776a6caa60c1049ca2623e40f`。
- 可打出清单：`probes/batch486_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round44.md`。
