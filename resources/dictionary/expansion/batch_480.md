# 批次 480：家电小故障与上门预约聊天

- 条数：373；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 373 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 340 个词按字记录在 `batch480_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`39a4632b8046b39b49b821d26761092441ff09d5e1996d0d5966a8030528f48c`。
- 可打出清单：`probes/batch480_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round43.md`。
