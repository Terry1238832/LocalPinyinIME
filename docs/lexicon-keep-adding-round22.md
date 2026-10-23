# 词库续加第22轮（批次 334–341）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `8c5e4c7d8e1e2db904b279f39ceefade98083f38`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：225166 → 226657，新增 **1491 条**。
- 权重 6000 / 5500 / 5000：0 / 1488 / 3 条。
- 最终 SHA-256：`0F8F2612BFB4F61A921C6D7135E555691BED0F423CC0F4A8C461AE3882119D4C`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

在原清单完成后继续提出日常延伸主题：演讲辩论与口头表达用于课堂发言、活动主持和业余辩论，聚焦口头表达而非上一轮语文句法；商超理货与零售门店用于店员交接、陈列补货和收银盘点，区别于购物消费者用语、餐饮门店和仓储物流；新闻阅读与信息辨识用于查原文、看统计口径和核对图文，不收具体新闻事件、人物或机构名；志愿服务与公益活动用于招募报名、时长登记和物资捐赠，不收组织名，不给募捐或医疗建议。每个主题在历史全层词面去重后筛选，不凭短语模板凑条数。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 334 | 演讲辩论与口头表达 | curate | 372 | 0 | 370 | 2 | 97.31% | 100.00% | `f10730cdf441b53129943eab295740668ac91a71` |
| 335 | 商超理货与零售门店 | curate | 364 | 0 | 363 | 1 | 95.05% | 100.00% | `769aeb65b2d896c39a8ac8e8bc3a6fe7d08d2f6c` |
| 336 | 新闻阅读与信息辨识 | curate | 353 | 0 | 353 | 0 | 96.03% | 100.00% | `431bc8ba3cd4aa1ab860fd38c3e688901988b57a` |
| 337 | 志愿服务与公益活动 | curate | 322 | 0 | 322 | 0 | 95.96% | 100.00% | `584bbbc9267b506a4949cad25d0d79506424a9de` |
| 338 | 演讲辩论与口头表达较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 100.00% | `2bf5c9e7a785d959f36d152096f685c038df3920` |
| 339 | 商超理货与零售门店较长说法 | merge | 20 | 0 | 20 | 0 | 55.00% | 100.00% | `271915262affc101d710e3d720351e5d1df5090a` |
| 340 | 新闻阅读与信息辨识较长说法 | merge | 20 | 0 | 20 | 0 | 90.00% | 100.00% | `160a9b75ad9022501113b019860b14df2e18c439` |
| 341 | 志愿服务与公益活动较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `b5a26958ab8dc1a39f29fc12dab81a8ba0311b2d` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round22_exclusions.tsv`，共 1513 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality:unnatural_or_redundant_label`：389 条。
- `duplicate:standard_zh_pinyin.tsv`：89 条。
- `quality:incomplete_or_unnatural_label`：488 条。
- `duplicate:local_core_zh_pinyin.tsv`：479 条。
- `pronunciation:contextual_reading_uncertain_omit`：1 条。
- `duplicate:batch334`：3 条。
- `batch_size_limit`：63 条。
- `duplicate:batch335`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1066 个含多音字的词在各批复核表逐字记录；3 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 演讲辩论：重音、重点的重 zhong，重复强调的重 chong；语调、强调、舞台调度的调 diao，调整的调 tiao；发言时长的长 chang，队长的长 zhang。
- 零售：店长的长 zhang；调货、调拨的调 diao，调整陈列的调 tiao。称重的称 cheng，复秤等不常用标签不收；近效期、临期按完整词义筛选。
- 新闻：调查的调 diao，旧闻重发、旧事重提的重 chong，重要的重 zhong；披露的露 lu，澄清的澄 cheng，转述与转发的转 zhuan。
- 公益：调班、调换的调 diao，调整排班的调 tiao；志愿者队长的长 zhang，服务时长的长 chang。轻声按本字写，组织称谓只收通用类别，不收具体机构名。
- “这个公益项目的善款去向公开吗”中结构助词“的”按 de 写，纠正默认 di；有把握不足的“卡壳”候选整词剔除。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1284/1284 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，226657 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round22_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.96% → 85.96% | +0.00 | 98.09% → 98.09% |
| 逐词输入·无前文 | 20626 | 77.97% → 77.97% | +0.00 | 97.17% → 97.17% |
| 留出整句 | 4126 | 58.05% → 58.05% | +0.00 | 73.41% → 73.44% |
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
- `docs/lexicon-keep-adding-round22.md`
- `resources/dictionary/expansion/batch334_authored_draft.txt`
- `resources/dictionary/expansion/batch334_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch335_authored_draft.txt`
- `resources/dictionary/expansion/batch335_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch336_authored_draft.txt`
- `resources/dictionary/expansion/batch336_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch337_authored_draft.txt`
- `resources/dictionary/expansion/batch337_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch338_draft.txt`
- `resources/dictionary/expansion/batch338_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch339_draft.txt`
- `resources/dictionary/expansion/batch339_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch340_draft.txt`
- `resources/dictionary/expansion/batch340_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch341_draft.txt`
- `resources/dictionary/expansion/batch341_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_334.md`
- `resources/dictionary/expansion/batch_335.md`
- `resources/dictionary/expansion/batch_336.md`
- `resources/dictionary/expansion/batch_337.md`
- `resources/dictionary/expansion/batch_338.md`
- `resources/dictionary/expansion/batch_339.md`
- `resources/dictionary/expansion/batch_340.md`
- `resources/dictionary/expansion/batch_341.md`
- `resources/dictionary/expansion/lexicon_batch_334.applied.json`
- `resources/dictionary/expansion/lexicon_batch_334.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_334.json`
- `resources/dictionary/expansion/lexicon_batch_334.tsv`
- `resources/dictionary/expansion/lexicon_batch_334.validation.json`
- `resources/dictionary/expansion/lexicon_batch_335.applied.json`
- `resources/dictionary/expansion/lexicon_batch_335.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_335.json`
- `resources/dictionary/expansion/lexicon_batch_335.tsv`
- `resources/dictionary/expansion/lexicon_batch_335.validation.json`
- `resources/dictionary/expansion/lexicon_batch_336.applied.json`
- `resources/dictionary/expansion/lexicon_batch_336.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_336.json`
- `resources/dictionary/expansion/lexicon_batch_336.tsv`
- `resources/dictionary/expansion/lexicon_batch_336.validation.json`
- `resources/dictionary/expansion/lexicon_batch_337.applied.json`
- `resources/dictionary/expansion/lexicon_batch_337.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_337.json`
- `resources/dictionary/expansion/lexicon_batch_337.tsv`
- `resources/dictionary/expansion/lexicon_batch_337.validation.json`
- `resources/dictionary/expansion/lexicon_batch_338.json`
- `resources/dictionary/expansion/lexicon_batch_338.tsv`
- `resources/dictionary/expansion/lexicon_batch_339.json`
- `resources/dictionary/expansion/lexicon_batch_339.tsv`
- `resources/dictionary/expansion/lexicon_batch_340.json`
- `resources/dictionary/expansion/lexicon_batch_340.tsv`
- `resources/dictionary/expansion/lexicon_batch_341.json`
- `resources/dictionary/expansion/lexicon_batch_341.tsv`
- `resources/dictionary/expansion/probes/batch334_probe.tsv`
- `resources/dictionary/expansion/probes/batch335_probe.tsv`
- `resources/dictionary/expansion/probes/batch336_probe.tsv`
- `resources/dictionary/expansion/probes/batch337_probe.tsv`
- `resources/dictionary/expansion/probes/batch338_probe.tsv`
- `resources/dictionary/expansion/probes/batch339_probe.tsv`
- `resources/dictionary/expansion/probes/batch340_probe.tsv`
- `resources/dictionary/expansion/probes/batch341_probe.tsv`
- `resources/dictionary/expansion/round22_exclusions.tsv`
- `resources/dictionary/expansion/round22_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
