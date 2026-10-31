# 批次 611：追剧听歌、观影和演出后的长句闲聊

- 条数：15；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 15 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 15 个词按字记录在 `batch611_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`f5b7b4bf5405b9609d45d7d7994a2726d2ccf73c5a62007e860c9327064992ad`。
- 可打出清单：`probes/batch611_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round64.md`。
