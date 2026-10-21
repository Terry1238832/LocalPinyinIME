# 批次 176：购物与生活服务较长说法

- 条数：6
- 来源：项目自拟（first-party）。按日常会整词打的说法逐条写，没有整库导入，没有从 `tests/data/` 取词。
- 标注：first_party_individually_authored。
- 权重：只使用 6000 / 5500 / 5000。已有同音词权重不低于 5000 时，新词降到不超过对方的档。分布：6000 有 0 条，5500 有 1 条，5000 有 5 条。
- 复核：含多音字的 5 个词在 `batch176_pronunciation_review.tsv` 里按字记录读音。和 pypinyin 默认键不一致的 0 条，决定栏标为 semantic。
- 这些词超过 6 个字，`curate_lexicon.py` 不收。用 `merge_lexicon_batch.py` 合并。evaluate 需要 Windows 上的 LocalPinyinImeEval.exe，未跑。

冻结 TSV SHA-256：`51423a091e3e0fe57d7e6b3f9998f3fc8ac5f9ede715901b0de4594290de122f`。
可打出清单：`probes/batch176_probe.tsv`。
