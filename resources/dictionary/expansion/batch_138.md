# 批次 138：著名地名、地标与国家和地区

- 条数：156
- 来源：jieba dict.txt（MIT）
- 标注：reference-derived。不是整库导入，也不是项目自拟。
- 权重：只使用项目已有粗档。 词语 6000 / 5500 / 5000；与已有同音词冲突时降到仍低于对方的档，降不下去则不收。
- 复核：逐条核对。含多音字的词在 `batch138_pronunciation_review.tsv` 里按字记录读音；单字读音采用《通用规范汉字表》标准读音（缺口表中的 kTGHZ2013），不采用 pypinyin 默认读音代替标准读音。
- 多读音单字每个标准读音一条。ü 写作 v，üe 写作 ue。
- 未跑 Windows 原生候选覆盖。

## 分类

- `well_known_places`：156

冻结 TSV SHA-256：`f3703d65895950c06e0ae41246a8dfc0c880c2cc85800bb7a22ea0aa45cb6f8a`。

