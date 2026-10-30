# 批次 602：日常推荐、试用体验与使用反馈聊天

- 条数：360；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 360 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 332 个词按字记录在 `batch602_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`6384e31be6ad48dc50147077388aca77261e82c04e3c9247a689a4e58bc6d2e8`。
- 可打出清单：`probes/batch602_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round63.md`。
