# 批次 158：四字成语上

- 条数：950
- 来源：jieba `dict.txt`（MIT）。从缺词清单排名 30,001–100,000、词性为成语（i）的四字词里逐条选用。不是整库导入。
- 标注：reference-derived。
- 权重：5000。已有同音词不低于 5000 时，不超过对方的档。
- 复核：多音字写入 `batch158_pronunciation_review.tsv`。默认读音保留的标为 context。改读音的只有：谁作 shei，作“像”讲的似作 si，必得的得作 de。
- 7 字以上不走 `curate_lexicon.py`。本批用 `merge_lexicon_batch.py` 合并。evaluate 需要 Windows 上的 LocalPinyinImeEval.exe，未跑。

冻结 TSV SHA-256：`e59056854a23cdfbe8bd6250e8c41351267cd71cdfc9eae2259610f5b14debef`。
