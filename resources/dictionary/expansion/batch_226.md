# 批次 226：图书馆与阅读较长说法

- 条数：12
- 来源：项目自拟（first-party）。按日常会整词打的说法逐条写，没有整库导入，没有从 `tests/data/` 取词。
- 标注：first_party_individually_authored。
- 权重：只使用 6000 / 5500 / 5000。已有同音词权重不低于 5000 时，新词降到不超过对方的档。分布：6000 有 0 条，5500 有 12 条，5000 有 0 条。
- 复核：含多音字的 10 个词在 `batch226_pronunciation_review.tsv` 里按字记录读音。和 pypinyin 默认键不一致的 1 条，决定栏标为 semantic。
- 这些词超过 6 个字，`curate_lexicon.py` 不收。用 `merge_lexicon_batch.py` 合并。evaluate 需要 Windows 上的 LocalPinyinImeEval.exe，未跑。

冻结 TSV SHA-256：`83f4a8b189d3c138510a64ef2881e231d356594a823b1dbac80d0a310391b253`。
可打出清单：`probes/batch226_probe.tsv`。
