# 批次 262：电竞

- 条数：438；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 437 / 1 条；同音权重上限导致 1 条降档。
- 含多音字的 301 个词按字记录在 `batch262_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`e8348d9deedecc428bb480b3c6c2bd68f80112d996086e78429f60289093b8c1`。
- 可打出清单：`probes/batch262_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round13.md`。
