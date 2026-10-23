# 批次 411：请求与拒绝日常表达补漏

- 条数：185；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 185 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 167 个词按字记录在 `batch411_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`2120a5bde6cf51ed58c3b56deb2b8ecbae7dab439e34fc75facb8420111f8567`。
- 可打出清单：`probes/batch411_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round32.md`。
