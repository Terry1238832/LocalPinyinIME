# 批次 161：常见谚语

- 条数：24
- 来源：jieba `dict.txt`（MIT）。缺词清单排名 30,001–100,000 里的常见长谚语，逐条选用。不是整库导入。
- 标注：reference-derived。
- 权重：5000。已有同音词不低于 5000 时，不超过对方的档。
- 复核：多音字写入 `batch161_pronunciation_review.tsv`。读音按整句核对，摸不着的着取 zhao，后天下之乐的乐取 le。
- 这些词超过 6 字，`curate_lexicon.py` 不收。用 `merge_lexicon_batch.py` 合并。evaluate 需要 Windows 上的 LocalPinyinImeEval.exe，未跑。

冻结 TSV SHA-256：`76efc256e93dde820135142fa8322a9ab990b055d4e6c0a94b51fb45fd3d2486`。
