# 批次 638：日常聊天：家常共餐、照顾口味与饭后收尾

- 条数：360；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 360 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 326 个词按字记录在 `batch638_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`23674393af08111929863212fc6476a4b1fcaba86ac4dcaf54f4e626c53add46`。
- 可打出清单：`probes/batch638_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round69.md`。
