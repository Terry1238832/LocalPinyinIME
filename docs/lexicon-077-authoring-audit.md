# 0.7.7 冻结词库结构审计

0.7.6 固定提交加冻结批次逐字节重建通过；原创补词 16,254，有效键词对 185,127。

旧本地词和权重、旧来源记录及另外四份运行数据保留。267 个公开整句答案排除；权重在候选测试前按类别固定。

逐词语境校音由单一 AI 作者完成，结构检查不等于第二名语言编辑，也不等于所有长句均能直接命中。

| 批次 | 新词 | 语境读音记录 | 语义修正 | 合法主读音选择 |
| --- | ---: | ---: | ---: | ---: |
| lexicon_batch_037 | 585 | 429 | 12 | 0 |
| lexicon_batch_038 | 699 | 534 | 26 | 4 |
| lexicon_batch_039 | 662 | 589 | 10 | 1 |
| lexicon_batch_040 | 550 | 470 | 7 | 2 |
| lexicon_batch_041 | 721 | 603 | 0 | 0 |
| lexicon_batch_042 | 704 | 566 | 19 | 0 |
| lexicon_batch_043 | 706 | 589 | 5 | 2 |
| lexicon_batch_044 | 732 | 686 | 4 | 0 |
| lexicon_batch_045 | 716 | 640 | 10 | 1 |
| lexicon_batch_046 | 735 | 713 | 51 | 0 |
| lexicon_batch_047 | 735 | 674 | 5 | 0 |
| lexicon_batch_048 | 706 | 630 | 23 | 17 |
| lexicon_batch_049 | 737 | 612 | 13 | 0 |
| lexicon_batch_050 | 746 | 705 | 40 | 2 |
| lexicon_batch_051 | 644 | 559 | 11 | 0 |
| lexicon_batch_052 | 577 | 504 | 10 | 0 |
| lexicon_batch_053 | 591 | 537 | 6 | 2 |
| lexicon_batch_054 | 577 | 524 | 11 | 0 |
| lexicon_batch_055 | 586 | 508 | 23 | 0 |
| lexicon_batch_056 | 509 | 464 | 37 | 0 |
| lexicon_batch_057 | 470 | 433 | 8 | 0 |
| lexicon_batch_058 | 492 | 447 | 11 | 1 |
| lexicon_batch_059 | 528 | 483 | 5 | 3 |
| lexicon_batch_060 | 519 | 475 | 9 | 5 |
| lexicon_batch_061 | 520 | 474 | 3 | 0 |
| lexicon_batch_062 | 507 | 471 | 4 | 0 |

[完整证据](lexicon-077-authoring-audit.json)。此前导入词库的来源和许可状态继承，不重新标记为本轮原创。
