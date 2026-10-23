# 批次 359：录音配音与声音处理

- 条数：305；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 305 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 194 个词按字记录在 `batch359_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`ff3c0808d49c436092fc90d5b5e1dd7d75ae2c5451e8f690c6eef71087b9d536`。
- 可打出清单：`probes/batch359_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round25.md`。
