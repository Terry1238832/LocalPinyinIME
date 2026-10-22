# 批次 210：社区与垃圾分类较长说法

- 条数：20
- 来源：项目自拟（first-party）。按日常会整词打的说法逐条写，没有整库导入，没有从 `tests/data/` 取词。
- 标注：first_party_individually_authored。
- 权重：只使用 6000 / 5500 / 5000。已有同音词权重不低于 5000 时，新词降到不超过对方的档。分布：6000 有 2 条，5500 有 11 条，5000 有 7 条。
- 复核：含多音字的 20 个词在 `batch210_pronunciation_review.tsv` 里按字记录读音。和 pypinyin 默认键不一致的 0 条，决定栏标为 semantic。
- 这些词超过 6 个字，`curate_lexicon.py` 不收。用 `merge_lexicon_batch.py` 合并。evaluate 需要 Windows 上的 LocalPinyinImeEval.exe，未跑。

冻结 TSV SHA-256：`2e607d7a375a4b27fbddafa3ca422b6f45bac4756e28aa0e0adc60a3376268e8`。
可打出清单：`probes/batch210_probe.tsv`。
