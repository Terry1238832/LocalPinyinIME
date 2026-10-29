# 批次 422：日常互助与临时小事口语

- 条数：427；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 427 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 379 个词按字记录在 `batch422_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`7a985655fa08eeff339b79755cd2ea5f1db9d27e34fa9b409cd4ee3a28341e4b`。
- 可打出清单：`probes/batch422_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round35.md`。
