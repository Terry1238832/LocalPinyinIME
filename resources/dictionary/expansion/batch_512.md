# 批次 512：桌游棋牌与对局商量较长说法

- 条数：20；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 20 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 20 个词按字记录在 `batch512_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`3b42b654890233a16820ad84a5b1fb83c34872af117e55b7ca0144da7532b016`。
- 可打出清单：`probes/batch512_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round48.md`。
