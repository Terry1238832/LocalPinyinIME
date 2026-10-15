# 0.7.8 冻结词库结构审计

0.7.7 固定提交加冻结批次逐字节重建通过；原创补词 11,775，有效键词对 196,902。

旧本地词和权重、旧来源记录及另外四份运行数据保留。267 个公开整句答案排除；权重在候选测试前按类别固定。

逐词语境校音由单一 AI 作者完成，结构检查不等于第二名语言编辑，也不等于所有长句均能直接命中。

| 批次 | 新词 | 语境读音记录 | 语义修正 | 合法主读音选择 |
| --- | ---: | ---: | ---: | ---: |
| lexicon_batch_063 | 436 | 344 | 10 | 0 |
| lexicon_batch_064 | 476 | 468 | 25 | 2 |
| lexicon_batch_065 | 440 | 300 | 0 | 0 |
| lexicon_batch_066 | 403 | 378 | 14 | 0 |
| lexicon_batch_067 | 407 | 368 | 2 | 0 |
| lexicon_batch_068 | 261 | 229 | 8 | 0 |
| lexicon_batch_069 | 321 | 263 | 2 | 0 |
| lexicon_batch_070 | 294 | 294 | 4 | 2 |
| lexicon_batch_071 | 289 | 268 | 8 | 0 |
| lexicon_batch_072 | 270 | 232 | 7 | 1 |
| lexicon_batch_073 | 347 | 326 | 8 | 0 |
| lexicon_batch_074 | 265 | 221 | 6 | 0 |
| lexicon_batch_075 | 350 | 277 | 4 | 0 |
| lexicon_batch_076 | 329 | 272 | 8 | 0 |
| lexicon_batch_077 | 258 | 247 | 8 | 0 |
| lexicon_batch_078 | 360 | 319 | 11 | 0 |
| lexicon_batch_079 | 283 | 171 | 5 | 0 |
| lexicon_batch_080 | 303 | 279 | 4 | 3 |
| lexicon_batch_081 | 320 | 245 | 6 | 0 |
| lexicon_batch_082 | 296 | 273 | 1 | 0 |
| lexicon_batch_083 | 224 | 194 | 2 | 0 |
| lexicon_batch_084 | 347 | 270 | 0 | 0 |
| lexicon_batch_085 | 290 | 233 | 4 | 1 |
| lexicon_batch_086 | 286 | 231 | 0 | 0 |
| lexicon_batch_087 | 355 | 345 | 5 | 0 |
| lexicon_batch_088 | 318 | 230 | 3 | 0 |
| lexicon_batch_089 | 307 | 265 | 1 | 0 |
| lexicon_batch_090 | 307 | 286 | 9 | 0 |
| lexicon_batch_091 | 326 | 326 | 14 | 1 |
| lexicon_batch_092 | 325 | 286 | 9 | 0 |
| lexicon_batch_093 | 305 | 275 | 5 | 0 |
| lexicon_batch_094 | 315 | 241 | 13 | 0 |
| lexicon_batch_095 | 349 | 347 | 1 | 1 |
| lexicon_batch_096 | 308 | 251 | 37 | 0 |
| lexicon_batch_097 | 280 | 215 | 15 | 0 |
| lexicon_batch_098 | 277 | 158 | 5 | 3 |
| lexicon_batch_099 | 148 | 99 | 0 | 0 |

[完整证据](lexicon-078-authoring-audit.json)。此前导入词库的来源和许可状态继承，不重新标记为本轮原创。
