# 批次 281：天文与科普常识

- 条数：350；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 349 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 263 个词按字记录在 `batch281_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`cdb5677658f866bd85c9abe4659da1f4cc68f7d7d38e89e4f6bc47981171e7c7`。
- 可打出清单：`probes/batch281_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round15.md`。
