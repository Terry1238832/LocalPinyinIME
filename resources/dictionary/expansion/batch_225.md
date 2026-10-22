# 批次 225：家政服务

- 条数：880
- 来源：项目自拟（first-party）。按日常会整词打的说法逐条写，没有整库导入，没有从 `tests/data/` 取词。
- 标注：first_party_individually_authored。
- 权重：只使用 6000 / 5500 / 5000。已有同音词权重不低于 5000 时，新词降到不超过对方的档。分布：6000 有 7 条，5500 有 615 条，5000 有 258 条。
- 复核：含多音字的 755 个词在 `batch225_pronunciation_review.tsv` 里按字记录读音。和 pypinyin 默认键不一致的 1 条，决定栏标为 semantic。
- `curate_lexicon.py` 已跑 prepare、freeze、apply。evaluate 需要 Windows 上的 LocalPinyinImeEval.exe，未跑。

冻结后的可打出清单：`probes/batch225_probe.tsv`。
