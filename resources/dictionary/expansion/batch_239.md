# 批次 239：电脑与网络日常

- 条数：320；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 320 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 271 个词按字记录在 `batch239_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：curate prepare → 读音复核 → freeze → apply → evaluate。
- 冻结 SHA-256：`a2e7273e249d3da7cccc1029a865d6b318c77d2f142f2bb9835764e8648446e6`。
- 可打出清单：`probes/batch239_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round10.md`。

## 第33轮自查勘误

弹窗的弹按“弹出窗口”读 tan，修正全拼与逐字复核、当前批次哈希和探针；词面与权重不变。原始追加回执的冻结哈希及 canonical 前后哈希保留历史含义，当前修订见 `round33_corrections.json` 和 `.json` 的 `correction_history`。重新运行原生候选检查，结果见第33轮记录。
