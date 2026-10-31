# 批次 597：语音留言分段发送、收听与补充长句

- 条数：25；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 25 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 25 个词按字记录在 `batch597_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`af5c60bb3ef383b8dd0389cacbe66885e39e852ab5c4697914244c33f71f7cb4`。
- 可打出清单：`probes/batch597_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round62.md`。
