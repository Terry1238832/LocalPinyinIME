# 批次 401：客套话与礼貌用语较长说法补漏

- 条数：10；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 10 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 10 个词按字记录在 `batch401_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`59964ffcd2a824262fb3f3ad0015e009a91b691e7932f1ecde1ae2835b116add`。
- 可打出清单：`probes/batch401_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round30.md`。
