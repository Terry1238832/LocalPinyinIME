# 批次 605：询问推荐、分享试用体验与购买建议长句

- 条数：23；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 23 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 23 个词按字记录在 `batch605_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`a83dd35cddec7c8634c0c22e25d9fdc2fd2dd8ad2e0dadf709c2f6918f725b81`。
- 可打出清单：`probes/batch605_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round63.md`。
