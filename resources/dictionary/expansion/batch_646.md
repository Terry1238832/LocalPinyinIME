# 批次 646：日常聊天：宠物近况、照看托付与日常互动

- 条数：376；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 376 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 333 个词按字记录在 `batch646_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`d7dda647fecf9727b71a2b36f99db02b4e5801b2971db2bb62345ae8f3a538ed`。
- 可打出清单：`probes/batch646_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round70.md`。
