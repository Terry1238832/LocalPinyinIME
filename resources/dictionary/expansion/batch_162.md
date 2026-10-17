# 批次 162：日常聊天短句

- 条数：175（常用 89 条 6000，其余 86 条 5500）
- 来源：项目自拟（MIT）。即时聊天里的问句、回复、语气词组合、“转我”“发你”这类代词短语、情绪与吐槽、约饭出行。不是整库导入，没有从 `tests/data/` 取词。
- 去重：已在任何一层词库里的短语不收，已有词条和权重都不变。
- 留出：与 `tests/data/colloquial_test.tsv` 某一句相同、或是其中三字以上片段的短语不收，保证这组日常口语测试仍是留出的。
- 复核：多音字写入 `batch162_pronunciation_review.tsv`（116 条），15 条按聊天用法指定读音，例如干嘛 ganma、得了吧 deleba、待会儿 daihuier、了解了 liaojiele。
- 用 `merge_lexicon_batch.py` 合并；可打出清单 `probes/batch162_probe.tsv`。

冻结 TSV SHA-256：`69a1ece8206c786f19c6e441e57c64ea95c9330b6699f91c548c3e924d8fe923`。
