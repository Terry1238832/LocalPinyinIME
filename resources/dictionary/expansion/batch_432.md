# 批次 432：吃饭点单与餐桌聊天较长说法

- 条数：19；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 19 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 19 个词按字记录在 `batch432_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`602392e486d644ec2fdb02ac15a8d730396e3f0b4c67710ff962c03761659982`。
- 可打出清单：`probes/batch432_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round36.md`。
