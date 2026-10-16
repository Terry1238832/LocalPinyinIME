# 批次 144：港台常用语

- 条数：14
- 来源：项目自拟（first-party）。这些词不在 jieba 前 3 万缺词清单里，来自覆盖率报告点名的时事、港台和科技短板。
- 标注：first_party_individually_authored。不是整库导入。
- 权重：只使用 6000 / 5500 / 5000。与已有同音词冲突时降到仍低于对方的档。
- 复核：含多音字的 13 个词在 `batch144_pronunciation_review.tsv` 里按字记录读音。默认键与选定读音一致，没有改成另一套读音。
- `curate_lexicon.py` 已跑 prepare、freeze、apply。evaluate 需要 Windows 上的 LocalPinyinImeEval.exe，未跑。

## 分类

- `hk_tw_common`：11
- `hk_tw_high`：2
- `hk_tw_lower`：1

冻结 TSV SHA-256：`58f636a14867c140cc6dc521c8e2b3581e2459984ca62fea8fff1400f2d51a2d`。

