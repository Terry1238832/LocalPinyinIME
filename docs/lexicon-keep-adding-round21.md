# 词库续加第21轮（批次 326–333）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `5fa14f4af0c4bb37a59f16a07bd439681eda1586`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：223596 → 225166，新增 **1570 条**。
- 权重 6000 / 5500 / 5000：0 / 1557 / 13 条。
- 最终 SHA-256：`BFCD8C09396BF28C87555EBA9CB1A5E6A29796C413325FD87C24A70B00CBA2F4`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

用户主题清单已在第二十轮完成。本轮提出四个延伸主题，逐词核对全历史词面而不重复原有综合批次：语文语法与修辞用于学生作业、家校答疑和文字修改，聚焦汉语句法、词义和修辞，不重复图书借阅；地理地形与自然景观用于读图、自然科普和户外观察，聚焦地貌与基础地理，不重复天气预警和地图导航操作；博物馆参观与文物常识用于预约讲解、看展提问和辨识常见器物，不收具体博物馆或藏品专名；通信套餐与号码业务用于查资费、补换卡、销户和转网咨询，不重复手机权限及应用操作。均选择完整常见说法，不收专业冷僻细分名。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 326 | 语文语法与修辞 | curate | 380 | 0 | 378 | 2 | 87.11% | 100.00% | `fc7d1666479f5188ec22702f5109dd7bc6bb56e7` |
| 327 | 地理地形与自然景观 | curate | 381 | 0 | 380 | 1 | 88.19% | 100.00% | `872718d49920d5f21da985a32c2eb623d452e6e3` |
| 328 | 博物馆参观与文物常识 | curate | 347 | 0 | 338 | 9 | 83.86% | 100.00% | `b95da94b7981169d43300dfed45a4fb242e252e3` |
| 329 | 通信套餐与号码业务 | curate | 384 | 0 | 383 | 1 | 91.93% | 100.00% | `8a76b4be7d357520dfa9de2637e652c08b2277b1` |
| 330 | 语文语法与修辞较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `e51f626c781acec92aba7891c8b6bd4fb32e9dee` |
| 331 | 地理地形与自然景观较长说法 | merge | 18 | 0 | 18 | 0 | 88.89% | 100.00% | `ae961c8ede59148429735786d0bd7343d9204ec7` |
| 332 | 博物馆参观与文物常识较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `d2e38b8b0d56eae6749a1995728ce2cdb7c230c1` |
| 333 | 通信套餐与号码业务较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `01902ce894e8a5f4fbe8371d278fcda6d832a312` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round21_exclusions.tsv`，共 1448 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality:uncommon_or_incomplete_label`：256 条。
- `quality:over_specialized_or_unnatural_label`：429 条。
- `duplicate:local_core_zh_pinyin.tsv`：524 条。
- `duplicate:standard_zh_pinyin.tsv`：125 条。
- `accuracy_regression:5000_still_displaces_existing_correct_sentence`：1 条。
- `candidate_top9:homophone_cap_5000_prevents_raise`：1 条。
- `batch_size_limit`：112 条。

首次 probe 中“玉衣 yuyi”未进入前九。已有“雨衣”权重 5000，同全拼最低档限制新词最高只能为 5000，无法提高到 5500 或 6000。因此删除这个新词，在临时副本中重建本轮全部冻结文件与下游哈希链后重跑原生验收。不改旧词、旧权重或测试集；未因准确率撤回整批。

随后准确率对比发现“新增用例”从 42/67（62.69%）降至 41/67（61.19%），超过 0.2 个百分点阈值。隔离资源副本逐例比较定位为本轮“洋流 yangliu”改变整句切分排名。仅在诊断副本降至 5000 后仍为 41/67，删除该新词后恢复 42/67，所有该组排名与本轮基线一致。因此删除“洋流”，再次重建本轮冻结文件和哈希链并重跑两种架构、全部 probe、curate evaluate、覆盖率及完整前后准确率。没有按评测答案补词，也没有修改评测文件、旧词、旧权重或引擎。此次是两次明确单条修复，未撤回整批；连续准确率撤批轮数仍为零。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1110 个含多音字的词在各批复核表逐字记录；5 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 语文：句读的读 dou（若入选）；语调、声调的调 diao；程度轻重的重 zhong，重复、多重复句的重 chong。语法句法按完整词义逐字复核。
- 地理：地壳的壳 qiao，地壳运动与地壳抬升都不取 ke；背风坡的背 bei，山脊的脊 ji，冲积的冲 chong，岩溶的溶 rong。地理术语不收具体地名。
- 文博：收藏、藏品、馆藏的藏 cang；款识的识 zhi，拓片、拓本的拓 ta，开拓的拓 tuo。薄胎瓷的薄 bao；乐舞、编钟等按乐器语境选乐 yue。
- 通信：转网、转入、转出的转 zhuan，重新、重置的重 chong；套餐降级的降 jiang，银行缴费的行 hang。轻声按本字写，形近字和规范写法一并复核。
- 重叠式、重叠词的重 chong；连读变调、声调的调 diao，与调整的调 tiao 区分。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1306/1306 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，225166 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round21_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.96% → 85.96% | +0.00 | 98.10% → 98.09% |
| 逐词输入·无前文 | 20626 | 77.97% → 77.97% | +0.00 | 97.17% → 97.17% |
| 留出整句 | 4126 | 58.05% → 58.05% | +0.00 | 73.41% → 73.41% |
| 口语对话·整句 | 3368 | 80.43% → 80.43% | +0.00 | 91.81% → 91.81% |
| 口语对话·逐词 | 19504 | 94.12% → 94.12% | +0.00 | 99.50% → 99.50% |
| 项目留出 | 138 | 79.71% → 79.71% | +0.00 | 86.23% → 86.23% |
| 新增用例 | 67 | 62.69% → 62.69% | +0.00 | 77.61% → 77.61% |
| 独立用例 | 62 | 41.94% → 41.94% | +0.00 | 62.90% → 62.90% |
| 日常口语·开发 | 149 | 96.64% → 96.64% | +0.00 | 100.00% → 100.00% |
| 日常口语·留出 | 150 | 87.33% → 87.33% | +0.00 | 97.33% → 97.33% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round21.md`
- `resources/dictionary/expansion/batch326_authored_draft.txt`
- `resources/dictionary/expansion/batch326_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch327_authored_draft.txt`
- `resources/dictionary/expansion/batch327_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch328_authored_draft.txt`
- `resources/dictionary/expansion/batch328_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch329_authored_draft.txt`
- `resources/dictionary/expansion/batch329_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch330_draft.txt`
- `resources/dictionary/expansion/batch330_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch331_draft.txt`
- `resources/dictionary/expansion/batch331_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch332_draft.txt`
- `resources/dictionary/expansion/batch332_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch333_draft.txt`
- `resources/dictionary/expansion/batch333_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_326.md`
- `resources/dictionary/expansion/batch_327.md`
- `resources/dictionary/expansion/batch_328.md`
- `resources/dictionary/expansion/batch_329.md`
- `resources/dictionary/expansion/batch_330.md`
- `resources/dictionary/expansion/batch_331.md`
- `resources/dictionary/expansion/batch_332.md`
- `resources/dictionary/expansion/batch_333.md`
- `resources/dictionary/expansion/lexicon_batch_326.applied.json`
- `resources/dictionary/expansion/lexicon_batch_326.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_326.json`
- `resources/dictionary/expansion/lexicon_batch_326.tsv`
- `resources/dictionary/expansion/lexicon_batch_326.validation.json`
- `resources/dictionary/expansion/lexicon_batch_327.applied.json`
- `resources/dictionary/expansion/lexicon_batch_327.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_327.json`
- `resources/dictionary/expansion/lexicon_batch_327.tsv`
- `resources/dictionary/expansion/lexicon_batch_327.validation.json`
- `resources/dictionary/expansion/lexicon_batch_328.applied.json`
- `resources/dictionary/expansion/lexicon_batch_328.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_328.json`
- `resources/dictionary/expansion/lexicon_batch_328.tsv`
- `resources/dictionary/expansion/lexicon_batch_328.validation.json`
- `resources/dictionary/expansion/lexicon_batch_329.applied.json`
- `resources/dictionary/expansion/lexicon_batch_329.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_329.json`
- `resources/dictionary/expansion/lexicon_batch_329.tsv`
- `resources/dictionary/expansion/lexicon_batch_329.validation.json`
- `resources/dictionary/expansion/lexicon_batch_330.json`
- `resources/dictionary/expansion/lexicon_batch_330.tsv`
- `resources/dictionary/expansion/lexicon_batch_331.json`
- `resources/dictionary/expansion/lexicon_batch_331.tsv`
- `resources/dictionary/expansion/lexicon_batch_332.json`
- `resources/dictionary/expansion/lexicon_batch_332.tsv`
- `resources/dictionary/expansion/lexicon_batch_333.json`
- `resources/dictionary/expansion/lexicon_batch_333.tsv`
- `resources/dictionary/expansion/probes/batch326_probe.tsv`
- `resources/dictionary/expansion/probes/batch327_probe.tsv`
- `resources/dictionary/expansion/probes/batch328_probe.tsv`
- `resources/dictionary/expansion/probes/batch329_probe.tsv`
- `resources/dictionary/expansion/probes/batch330_probe.tsv`
- `resources/dictionary/expansion/probes/batch331_probe.tsv`
- `resources/dictionary/expansion/probes/batch332_probe.tsv`
- `resources/dictionary/expansion/probes/batch333_probe.tsv`
- `resources/dictionary/expansion/round21_exclusions.tsv`
- `resources/dictionary/expansion/round21_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
