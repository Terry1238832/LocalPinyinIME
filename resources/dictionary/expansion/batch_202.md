# 批次 202：影视短视频

- 条数：760
- 来源：项目自拟（first-party）。按日常会整词打的说法逐条写，没有整库导入，没有从 `tests/data/` 取词。
- 标注：first_party_individually_authored。
- 权重：只使用 6000 / 5500 / 5000。已有同音词权重不低于 5000 时，新词降到不超过对方的档。分布：6000 有 76 条，5500 有 427 条，5000 有 257 条。
- 复核：含多音字的 502 个词在 `batch202_pronunciation_review.tsv` 里按字记录读音。和 pypinyin 默认键不一致的 17 条，决定栏标为 semantic。
- `curate_lexicon.py` 已跑 prepare、freeze、apply。evaluate 需要 Windows 上的 LocalPinyinImeEval.exe，未跑。

冻结后的可打出清单：`probes/batch202_probe.tsv`。
