# 批次 185：办事、证件与法律常识较长说法

- 条数：80
- 来源：项目自拟（first-party）。按日常会整词打的说法逐条写，没有整库导入，没有从 `tests/data/` 取词。
- 标注：first_party_individually_authored。
- 权重：只使用 6000 / 5500 / 5000。已有同音词权重不低于 5000 时，新词降到不超过对方的档。分布：6000 有 7 条，5500 有 67 条，5000 有 6 条。
- 复核：含多音字的 74 个词在 `batch185_pronunciation_review.tsv` 里按字记录读音。和 pypinyin 默认键不一致的 1 条，决定栏标为 semantic。
- 这些词超过 6 个字，`curate_lexicon.py` 不收。用 `merge_lexicon_batch.py` 合并。evaluate 需要 Windows 上的 LocalPinyinImeEval.exe，未跑。

冻结 TSV SHA-256：`1007b4bada51ef00306a4f3f3927fd23a46eac89ae6e3bded224ad3e1ea30b6f`。
可打出清单：`probes/batch185_probe.tsv`。
