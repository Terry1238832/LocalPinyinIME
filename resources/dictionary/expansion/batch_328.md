# 批次 328：博物馆参观与文物常识

- 条数：347；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 338 / 9 条；同音权重上限导致 9 条降档。
- 含多音字的 206 个词按字记录在 `batch328_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`79232c834f368c8340044268f3cefb62ea2181ba8c489f5e719c587e8a989db0`。
- 可打出清单：`probes/batch328_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round21.md`。
