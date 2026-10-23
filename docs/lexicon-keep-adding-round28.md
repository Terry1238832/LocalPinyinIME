# 词库续加第28轮（批次 382–389）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `f9edf91d9bc8499dd859653c15d7a71a53386515`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：233539 → 234937，新增 **1398 条**。
- 权重 6000 / 5500 / 5000：0 / 1390 / 8 条。
- 最终 SHA-256：`EEBE2A9E76485449E5E643107DED72C8B77A33ABB6B92A4ABCC2FA595F7C113D`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

原主题清单已完成。本轮自拟纸艺折纸剪纸、昆虫自然观察、门窗窗帘遮阳、鞋履修护四个专题。分别服务亲子手作和节庆纸艺、儿童科普观察、家庭窗帘定制与使用、洗鞋修鞋及护鞋用品沟通。与既有文具、理科、家居家政、服饰主题有关，但没有同名专批；严格与历史词面去重，删去机械参数组合、罕见学术名目和不自然的整词表达。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 382 | 纸艺折纸与剪纸 | curate | 306 | 0 | 306 | 0 | 92.48% | 100.00% | `6167c3714288a088dc9181390672f9645502d320` |
| 383 | 昆虫与自然观察 | curate | 316 | 0 | 311 | 5 | 80.06% | 100.00% | `705f46dff0dc0d8c0dd3aa036f4f49dd5a6a210c` |
| 384 | 门窗窗帘与遮阳用品 | curate | 324 | 0 | 323 | 1 | 87.35% | 100.00% | `9380a758ca35c08a7796cba4ee12e9fdca68ebea` |
| 385 | 鞋履修护与日常护鞋 | curate | 372 | 0 | 370 | 2 | 91.94% | 100.00% | `74283a7bf9af1a63e9d9c9f7113b5197d54cc7e3` |
| 386 | 纸艺折纸与剪纸较长说法 | merge | 20 | 0 | 20 | 0 | 55.00% | 100.00% | `b4cd90bf6969406f4a77a6099e11d230d7366915` |
| 387 | 昆虫与自然观察较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `12bf75975ba074588cd45ab03377056566b57327` |
| 388 | 门窗窗帘与遮阳用品较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `83e02c1a93ee054d6e2ee69a56e49e15d2dfd0ee` |
| 389 | 鞋履修护与日常护鞋较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `ace9da7daf998b5a1fa36b85b1dcfd9e640987ac` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round28_exclusions.tsv`，共 1854 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：342 条。
- `quality: repetitive specification, narrow craft detail or unnatural composite`：279 条。
- `quality: less common craft or curtain label; uncertain whole-word usage`：44 条。
- `quality: nonstandard form, uncertain label, name-derived style or unsuitable niche expression`：22 条。
- `duplicate:standard_zh_pinyin.tsv`：57 条。
- `quality: remaining narrow service detail or awkward whole-input wording`：48 条。
- `probe_top9: zhiyu absent from top nine; omit specialist biology term rather than increase its ranking priority`：1 条。
- `quality: narrow taxonomy or anatomy, repetitive observation field or unnatural phrase`：285 条。
- `batch_size_limit`：261 条。
- `quality: repetitive dimension or component combination; not a common standalone input`：276 条。
- `quality: repetitive footwear description, narrow manufacturing detail or unnatural phrase`：215 条。
- `quality: repetitive footwear parameter or administrative label`：24 条。

首次探针中滞育 zhiyu 未进前九，本轮删除该较专门的生物学术语，不提权。随后从补词前基线重建未提交批次，重新完成双架构构建、全部探针、curate evaluate、准确率前后对比与结构校验。未撤回整批。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1170 个含多音字的词在各批复核表逐字记录；12 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐条复核全文拼音。折纸的折取 zhe、折错重来的重取 chong、纸浆的浆取 jiang、卷纸取 juan，纸张褪色取 se；昆虫蜕壳与蝉蜕壳的壳取 ke，卵鞘取 qiao，柞蚕取 zuo，蠼螋取 qu sou，螽斯取 zhong si，蜚蠊取 fei lian；窗帘太长与帘子太长取 chang，窗帘褪色取 se，窗帘杆取 gan，卡住取 ka；鞋带系法取 ji，可调鞋撑取 tiao，长靴鞋撑取 chang，鞋子/鞋面/皮鞋褪色及褪色测试取 se，鞋楦取 xuan，麂皮取 ji，鞋底粘合取 zhan。每个含多音字词在复核表逐字记录，不照搬默认拼读。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1142/1142 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，234937 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round28_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.95% → 85.97% | +0.02 | 98.09% → 98.11% |
| 逐词输入·无前文 | 20626 | 77.97% → 77.98% | +0.01 | 97.17% → 97.19% |
| 留出整句 | 4126 | 58.05% → 58.07% | +0.02 | 73.44% → 73.44% |
| 口语对话·整句 | 3368 | 80.40% → 80.40% | +0.00 | 91.81% → 91.81% |
| 口语对话·逐词 | 19504 | 94.12% → 94.12% | +0.00 | 99.50% → 99.50% |
| 项目留出 | 138 | 79.71% → 79.71% | +0.00 | 86.23% → 86.23% |
| 新增用例 | 67 | 62.69% → 62.69% | +0.00 | 77.61% → 77.61% |
| 独立用例 | 62 | 41.94% → 41.94% | +0.00 | 62.90% → 62.90% |
| 日常口语·开发 | 149 | 96.64% → 96.64% | +0.00 | 100.00% → 100.00% |
| 日常口语·留出 | 150 | 87.33% → 87.33% | +0.00 | 97.33% → 97.33% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round28.md`
- `resources/dictionary/expansion/batch382_authored_draft.txt`
- `resources/dictionary/expansion/batch382_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch383_authored_draft.txt`
- `resources/dictionary/expansion/batch383_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch384_authored_draft.txt`
- `resources/dictionary/expansion/batch384_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch385_authored_draft.txt`
- `resources/dictionary/expansion/batch385_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch386_draft.txt`
- `resources/dictionary/expansion/batch386_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch387_draft.txt`
- `resources/dictionary/expansion/batch387_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch388_draft.txt`
- `resources/dictionary/expansion/batch388_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch389_draft.txt`
- `resources/dictionary/expansion/batch389_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_382.md`
- `resources/dictionary/expansion/batch_383.md`
- `resources/dictionary/expansion/batch_384.md`
- `resources/dictionary/expansion/batch_385.md`
- `resources/dictionary/expansion/batch_386.md`
- `resources/dictionary/expansion/batch_387.md`
- `resources/dictionary/expansion/batch_388.md`
- `resources/dictionary/expansion/batch_389.md`
- `resources/dictionary/expansion/lexicon_batch_382.applied.json`
- `resources/dictionary/expansion/lexicon_batch_382.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_382.json`
- `resources/dictionary/expansion/lexicon_batch_382.tsv`
- `resources/dictionary/expansion/lexicon_batch_382.validation.json`
- `resources/dictionary/expansion/lexicon_batch_383.applied.json`
- `resources/dictionary/expansion/lexicon_batch_383.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_383.json`
- `resources/dictionary/expansion/lexicon_batch_383.tsv`
- `resources/dictionary/expansion/lexicon_batch_383.validation.json`
- `resources/dictionary/expansion/lexicon_batch_384.applied.json`
- `resources/dictionary/expansion/lexicon_batch_384.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_384.json`
- `resources/dictionary/expansion/lexicon_batch_384.tsv`
- `resources/dictionary/expansion/lexicon_batch_384.validation.json`
- `resources/dictionary/expansion/lexicon_batch_385.applied.json`
- `resources/dictionary/expansion/lexicon_batch_385.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_385.json`
- `resources/dictionary/expansion/lexicon_batch_385.tsv`
- `resources/dictionary/expansion/lexicon_batch_385.validation.json`
- `resources/dictionary/expansion/lexicon_batch_386.json`
- `resources/dictionary/expansion/lexicon_batch_386.tsv`
- `resources/dictionary/expansion/lexicon_batch_387.json`
- `resources/dictionary/expansion/lexicon_batch_387.tsv`
- `resources/dictionary/expansion/lexicon_batch_388.json`
- `resources/dictionary/expansion/lexicon_batch_388.tsv`
- `resources/dictionary/expansion/lexicon_batch_389.json`
- `resources/dictionary/expansion/lexicon_batch_389.tsv`
- `resources/dictionary/expansion/probes/batch382_probe.tsv`
- `resources/dictionary/expansion/probes/batch383_probe.tsv`
- `resources/dictionary/expansion/probes/batch384_probe.tsv`
- `resources/dictionary/expansion/probes/batch385_probe.tsv`
- `resources/dictionary/expansion/probes/batch386_probe.tsv`
- `resources/dictionary/expansion/probes/batch387_probe.tsv`
- `resources/dictionary/expansion/probes/batch388_probe.tsv`
- `resources/dictionary/expansion/probes/batch389_probe.tsv`
- `resources/dictionary/expansion/round28_exclusions.tsv`
- `resources/dictionary/expansion/round28_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`

## 后续选题评估与停止位置

本轮全部验收并推送后，停止于第二十八轮（批次 382–389），下一批号为 390。触发用户规定的停止项：主题清单已用尽，且作者无法有把握提出满足当前批量要求的新主题。

原清单已在第二十轮完成；第二十一至二十八轮又自拟了 32 个日常专题，并逐词去重与复核。本轮短词按主题筛选后保留的规模已接近每批 300 条的下限；初审曾有三个主题不足，重新审阅既有自拟草稿中的实际观察、报修与用品名称后才满足要求。未把被排除的冷僻词或参数组合补回来。

后续考虑过家居收纳、衣物洗护、缝纫修补、水族观察等方向，分别已有批次 024、124、098、099，不能重复开题；更细的设备参数、罕见物种或专门工艺名目，难以保证“现在会整词输入的常用说法”与每主题至少 300 条同时成立。这里记录的是本作者对后续自拟选题的把握不足，不声称汉语词库已经没有任何可补词。

准确率整批撤回累计为 0；停止由选题条件触发。本轮若有单词候选修复，详见上文排除与验收记录。全部已验收的批次均已按批提交并推送，未完成未来批次没有入库、提交或推送。

建议后续改为每专题 50–150 条的专项补漏，以实际使用中独立记录的常用缺词为起点，继续保持逐条自拟、测试隔离、读音复核和原生验收；或者先安排已有词条的独立读音审校。该建议需要另行明确任务范围，本次没有擅自改动条数要求或已有词条。
