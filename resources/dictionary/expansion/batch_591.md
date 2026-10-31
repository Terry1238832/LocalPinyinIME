# 批次 591：分享照片、链接和文件后确认查看情况长句

- 条数：24；first_party_individually_authored，逐条自拟，无第三方词库导入，无测试集取词。
- 权重：6000 / 5500 / 5000 分别 0 / 24 / 0 条；同音权重上限导致 0 条降档。
- 含多音字的 24 个词按字记录在 `batch591_pronunciation_review.tsv`，按语境复核；一名 AI 作者，不声称独立人工语言学审校。
- 流程：7 字以上使用 merge_lexicon_batch.py。
- 冻结 SHA-256：`73ce6f0577b91b7ae10de13fcedbe44b08aab3d516e5381bdcf8044cd19ad6e5`。
- 可打出清单：`probes/batch591_probe.tsv`。
- 原生验收、排除词及未运行步骤见 `docs/lexicon-keep-adding-round61.md`。
