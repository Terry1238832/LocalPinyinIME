# 0.7.9 冻结词库结构审计

0.7.8 固定提交加常用词优先级清单及冻结批次逐字节重建通过；原创补词 12,404，有效键词对 209,306。

旧本地词、旧来源记录及另外四份运行数据保留。118项常用词按单独清单提高优先级，其余旧权重保持。267 个公开整句答案排除；权重在候选测试前按类别固定。

逐词语境校音由单一 AI 作者完成，结构检查不等于第二名语言编辑，也不等于所有长句均能直接命中。

| 批次 | 新词 | 语境读音记录 | 语义修正 | 合法主读音选择 |
| --- | ---: | ---: | ---: | ---: |
| lexicon_batch_100 | 266 | 218 | 3 | 0 |
| lexicon_batch_101 | 351 | 319 | 25 | 3 |
| lexicon_batch_102 | 293 | 248 | 5 | 0 |
| lexicon_batch_103 | 386 | 360 | 11 | 1 |
| lexicon_batch_104 | 343 | 296 | 6 | 0 |
| lexicon_batch_105 | 380 | 284 | 7 | 0 |
| lexicon_batch_106 | 343 | 289 | 14 | 0 |
| lexicon_batch_107 | 328 | 284 | 14 | 0 |
| lexicon_batch_108 | 313 | 228 | 2 | 0 |
| lexicon_batch_109 | 386 | 325 | 9 | 1 |
| lexicon_batch_110 | 335 | 307 | 6 | 0 |
| lexicon_batch_111 | 362 | 281 | 6 | 0 |
| lexicon_batch_112 | 355 | 262 | 8 | 0 |
| lexicon_batch_113 | 400 | 332 | 1 | 1 |
| lexicon_batch_114 | 374 | 299 | 1 | 0 |
| lexicon_batch_115 | 255 | 198 | 4 | 0 |
| lexicon_batch_116 | 416 | 299 | 4 | 1 |
| lexicon_batch_117 | 423 | 311 | 5 | 0 |
| lexicon_batch_118 | 354 | 281 | 3 | 2 |
| lexicon_batch_119 | 316 | 281 | 8 | 0 |
| lexicon_batch_120 | 331 | 279 | 2 | 0 |
| lexicon_batch_121 | 356 | 278 | 7 | 0 |
| lexicon_batch_122 | 364 | 279 | 10 | 1 |
| lexicon_batch_123 | 357 | 306 | 5 | 1 |
| lexicon_batch_124 | 328 | 268 | 2 | 0 |
| lexicon_batch_125 | 349 | 313 | 2 | 0 |
| lexicon_batch_126 | 280 | 237 | 4 | 0 |
| lexicon_batch_127 | 455 | 334 | 1 | 7 |
| lexicon_batch_128 | 329 | 268 | 1 | 1 |
| lexicon_batch_129 | 465 | 348 | 11 | 0 |
| lexicon_batch_130 | 335 | 256 | 0 | 0 |
| lexicon_batch_131 | 360 | 264 | 4 | 0 |
| lexicon_batch_132 | 374 | 312 | 5 | 0 |
| lexicon_batch_133 | 348 | 245 | 1 | 0 |
| lexicon_batch_134 | 394 | 340 | 0 | 0 |

[完整证据](lexicon-079-authoring-audit.json)。此前导入词库的来源和许可状态继承，不重新标记为本轮原创。
