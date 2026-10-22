# 批次 198：升学与职业资格较长说法

- 条数：66
- 来源：项目自拟（first-party）。按日常会整词打的说法逐条写，没有整库导入，没有从 `tests/data/` 取词。
- 标注：first_party_individually_authored。
- 权重：只使用 6000 / 5500 / 5000。已有同音词权重不低于 5000 时，新词降到不超过对方的档。分布：6000 有 0 条，5500 有 36 条，5000 有 30 条。
- 复核：含多音字的 59 个词在 `batch198_pronunciation_review.tsv` 里按字记录读音。和 pypinyin 默认键不一致的 1 条，决定栏标为 semantic。
- 这些词超过 6 个字，`curate_lexicon.py` 不收。用 `merge_lexicon_batch.py` 合并。evaluate 需要 Windows 上的 LocalPinyinImeEval.exe，未跑。

冻结 TSV SHA-256：`944ec18792a0ec29ffd36209530246e5f71df058eb5c11400fd28bb73604b271`。
可打出清单：`probes/batch198_probe.tsv`。
