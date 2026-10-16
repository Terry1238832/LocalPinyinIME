# 批次 147：外国州名与地区名

- 条数：3
- 来源：项目自拟（first-party）。这三个地名不在 jieba 前 3 万缺词清单里，是覆盖率报告点名的外国地名短板。
- 标注：first_party_individually_authored。不是整库导入。
- 权重：只使用 6000 / 5500 / 5000。与已有同音词冲突时降到仍低于对方的档。
- 复核：含多音字的 2 个词在 `batch147_pronunciation_review.tsv` 里按字记录读音。默认键与选定读音一致，没有改成另一套读音。
- `curate_lexicon.py` 已跑 prepare、freeze、apply。evaluate 需要 Windows 上的 LocalPinyinImeEval.exe，未跑。

## 分类

- `foreign_places`：3

冻结 TSV SHA-256：`7abe1cdd830123683c07df7042251c27751869265676ad22995cf48a3476dd09`。

