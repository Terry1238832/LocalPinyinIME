# 词库续加第18轮（批次 302–309）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `6e0a33e6926073e4b5cf06b35e6e017999559deb`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：218303 → 220213，新增 **1910 条**。
- 权重 6000 / 5500 / 5000：0 / 1910 / 0 条。
- 最终 SHA-256：`5AEA6397B6ABE036D59A2A7D07A34BF5CC82B7AD91B3AA059E5E4C4491BA1105`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 302 | 减脂与健康饮食 | curate | 475 | 0 | 475 | 0 | 90.74% | 100.00% | `396f83029def5b084a8a19651ff6253754115021` |
| 303 | 早教与亲子 | curate | 462 | 0 | 462 | 0 | 91.34% | 100.00% | `199910b091e6095015323c8258d0d0011b5587ee` |
| 304 | 大学校园生活 | curate | 409 | 0 | 409 | 0 | 94.62% | 100.00% | `dfec52e3c0a8b64eb72fae309946022c1aac2c82` |
| 305 | 考研与考公 | curate | 484 | 0 | 484 | 0 | 86.78% | 100.00% | `49e86fbbfcbf838d7c02b90f536bdd2a8dc76e61` |
| 306 | 减脂与健康饮食较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `e2cca746ac40aeee5048659b8feafeb2c5532bd9` |
| 307 | 早教与亲子较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `970d66b95f610260c053c8ca5e6e4ffec8604f27` |
| 308 | 大学校园生活较长说法 | merge | 20 | 0 | 20 | 0 | 65.00% | 100.00% | `b12496cfebbe56aeb6bff4adb157d5f88deab101` |
| 309 | 考研与考公较长说法 | merge | 20 | 0 | 20 | 0 | 55.00% | 100.00% | `68dce925a40f0f77ab5d252107047aafa0a983fb` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round18_exclusions.tsv`，共 1040 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality:incomplete_or_uncommon_label`：110 条。
- `quality:unnatural_or_redundant_label`：90 条。
- `duplicate:local_core_zh_pinyin.tsv`：490 条。
- `outside_batch_length_scope`：2 条。
- `duplicate:standard_zh_pinyin.tsv`：34 条。
- `duplicate:batch302`：2 条。
- `quality:potential_product_marketing_name_omit`：1 条。
- `batch_size_limit`：310 条。
- `duplicate:batch303`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1433 个含多音字的词在各批复核表逐字记录；7 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 饮食：分量、重量、称重中的量 liang、重 zhong、称 cheng；减脂日常用语只作为词条，不承诺减重效果。标准书写优先“分量”。
- 亲子：调课及调到周末的调 diao，调整、协调的调 tiao；成长的长 zhang，时长的长 chang；数数、按物点数的数 shu，数量的数 shu（声调不写入全拼）。拓印的拓 ta。
- 校园：调换宿舍、调课、调档的调 diao；调剂、调整的调 tiao。重修的重 chong，体重的重 zhong。寝室长、楼层长、家长的长 zhang，长难句的长 chang。
- 备考：行测的行 xing；调剂的调 tiao，调档函的调 diao；参公的参 can，差额的差 cha，考察、考场按本字读音。轻声按本字写。
- 调查（如膳食调查）的调 diao；学生会部长的长 zhang，增长量的长 zhang。按完整词义复核，未沿用先前字级默认。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1422/1422 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，220213 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round18_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.96% → 85.96% | +0.00 | 98.10% → 98.10% |
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
- `docs/lexicon-keep-adding-round18.md`
- `resources/dictionary/expansion/batch302_authored_draft.txt`
- `resources/dictionary/expansion/batch302_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch303_authored_draft.txt`
- `resources/dictionary/expansion/batch303_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch304_authored_draft.txt`
- `resources/dictionary/expansion/batch304_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch305_authored_draft.txt`
- `resources/dictionary/expansion/batch305_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch306_draft.txt`
- `resources/dictionary/expansion/batch306_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch307_draft.txt`
- `resources/dictionary/expansion/batch307_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch308_draft.txt`
- `resources/dictionary/expansion/batch308_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch309_draft.txt`
- `resources/dictionary/expansion/batch309_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_302.md`
- `resources/dictionary/expansion/batch_303.md`
- `resources/dictionary/expansion/batch_304.md`
- `resources/dictionary/expansion/batch_305.md`
- `resources/dictionary/expansion/batch_306.md`
- `resources/dictionary/expansion/batch_307.md`
- `resources/dictionary/expansion/batch_308.md`
- `resources/dictionary/expansion/batch_309.md`
- `resources/dictionary/expansion/lexicon_batch_302.applied.json`
- `resources/dictionary/expansion/lexicon_batch_302.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_302.json`
- `resources/dictionary/expansion/lexicon_batch_302.tsv`
- `resources/dictionary/expansion/lexicon_batch_302.validation.json`
- `resources/dictionary/expansion/lexicon_batch_303.applied.json`
- `resources/dictionary/expansion/lexicon_batch_303.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_303.json`
- `resources/dictionary/expansion/lexicon_batch_303.tsv`
- `resources/dictionary/expansion/lexicon_batch_303.validation.json`
- `resources/dictionary/expansion/lexicon_batch_304.applied.json`
- `resources/dictionary/expansion/lexicon_batch_304.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_304.json`
- `resources/dictionary/expansion/lexicon_batch_304.tsv`
- `resources/dictionary/expansion/lexicon_batch_304.validation.json`
- `resources/dictionary/expansion/lexicon_batch_305.applied.json`
- `resources/dictionary/expansion/lexicon_batch_305.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_305.json`
- `resources/dictionary/expansion/lexicon_batch_305.tsv`
- `resources/dictionary/expansion/lexicon_batch_305.validation.json`
- `resources/dictionary/expansion/lexicon_batch_306.json`
- `resources/dictionary/expansion/lexicon_batch_306.tsv`
- `resources/dictionary/expansion/lexicon_batch_307.json`
- `resources/dictionary/expansion/lexicon_batch_307.tsv`
- `resources/dictionary/expansion/lexicon_batch_308.json`
- `resources/dictionary/expansion/lexicon_batch_308.tsv`
- `resources/dictionary/expansion/lexicon_batch_309.json`
- `resources/dictionary/expansion/lexicon_batch_309.tsv`
- `resources/dictionary/expansion/probes/batch302_probe.tsv`
- `resources/dictionary/expansion/probes/batch303_probe.tsv`
- `resources/dictionary/expansion/probes/batch304_probe.tsv`
- `resources/dictionary/expansion/probes/batch305_probe.tsv`
- `resources/dictionary/expansion/probes/batch306_probe.tsv`
- `resources/dictionary/expansion/probes/batch307_probe.tsv`
- `resources/dictionary/expansion/probes/batch308_probe.tsv`
- `resources/dictionary/expansion/probes/batch309_probe.tsv`
- `resources/dictionary/expansion/round18_exclusions.tsv`
- `resources/dictionary/expansion/round18_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
