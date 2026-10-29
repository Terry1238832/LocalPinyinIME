# 批次 513：拼图拼装与手作协作较长说法

- 条数：20；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 20 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 20 个词按字记录在 `batch513_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`fbd365326db7ad03e138a28ce0e3a1275d6600dff86577533b02a9c6ed62361a`。
- 可打出清单：`probes/batch513_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round48.md`。
