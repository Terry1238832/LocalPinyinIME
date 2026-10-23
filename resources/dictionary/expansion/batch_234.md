# 批次 234：音乐与乐器较长说法

- 条数：20
- 来源：项目逐条自拟（first-party authored），未导入第三方词库，未从 `tests/data/` 取词。
- 权重：6000 有 0 条，5500 有 14 条，5000 有 6 条。同全拼不超过已有不低于 5000 的最低档，0 条因此降档。
- 复核：含多音字的 17 个词在 `batch234_pronunciation_review.tsv` 里逐字记录；一名 AI 作者按语义复核，不声称独立人工语言学审校。
- 流程：7 字及以上独立成批，使用 `merge_lexicon_batch.py` 合并。
- 冻结 TSV SHA-256：`54d9900e7ff8a9f45f570aa98b0bb54384c870618b4d359c265a015cfceccc9b`。
- 可打出清单：`probes/batch234_probe.tsv`。
- 原生候选检查和准确率结果见 `docs/lexicon-keep-adding-round9.md`（整轮完成后记录）。
