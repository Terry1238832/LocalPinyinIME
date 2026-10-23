# 词库续加第23轮（批次 342–349）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `4b3556b097b7011018944f3e23cef0d25a1ae7d6`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：226657 → 228008，新增 **1351 条**。
- 权重 6000 / 5500 / 5000：0 / 1339 / 12 条。
- 最终 SHA-256：`8B8528AAC5F4561B58A3453E7597C36A99C9915B086BE015FBE1B48F51DB15D1`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

原清单已完成，继续评估日常兴趣与服务中的细分空缺：自行车与骑行用于骑友交流、通勤选车和维护报修；游泳与水上休闲用于泳池办卡、装备选购和水上体验咨询；钟表珠宝与佩戴养护用于调表、换表带、首饰清洁和圈口咨询；会展与线下活动筹备用于参展对接、会场布置和物料交接。早期综合批次仅覆盖其中部分出行、身体感受和文化活动，本轮按历史全部词面逐条去重，只补这些独立专题的日常余项，不收具体品牌、赛事、场馆、机构或人物名。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 342 | 自行车与骑行 | curate | 320 | 0 | 318 | 2 | 88.12% | 100.00% | `23c7c8c62e56de2ec04bc4ca19745e4556132814` |
| 343 | 游泳与水上休闲 | curate | 314 | 0 | 312 | 2 | 93.31% | 100.00% | `45ca2f87a2e4b88d5f9db1964a6365e555790587` |
| 344 | 钟表珠宝与佩戴养护 | curate | 331 | 0 | 326 | 5 | 90.94% | 100.00% | `ead4ded8773646e632515b6ef99821bee445ccf3` |
| 345 | 会展与线下活动筹备 | curate | 307 | 0 | 304 | 3 | 96.74% | 100.00% | `03530df3d2199a78ae4446b4a08a13ed600272b6` |
| 346 | 自行车与骑行较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `78bc76b927dd140a6bb78979f4ba2b3b9db036b4` |
| 347 | 游泳与水上休闲较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `ac243743e4c13ee20a130038d50fe1b50a679808` |
| 348 | 钟表珠宝与佩戴养护较长说法 | merge | 19 | 0 | 19 | 0 | 57.89% | 100.00% | `afee0ae459281ed7742519060f87e35ec532fd83` |
| 349 | 会展与线下活动筹备较长说法 | merge | 20 | 0 | 20 | 0 | 55.00% | 100.00% | `585c96d7e3176d9854e3987697d186c193b04a92` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round23_exclusions.tsv`，共 2169 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality:omit_redundant_or_over_specific_labels`：611 条。
- `quality:over_specific_or_unnatural_label`：768 条。
- `duplicate:local_core_zh_pinyin.tsv`：420 条。
- `duplicate:standard_zh_pinyin.tsv`：55 条。
- `batch_size_limit`：312 条。
- `outside_batch_length_scope`：1 条。
- `duplicate:batch343`：1 条。
- `pronunciation:contextual_reading_uncertain_omit`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

984 个含多音字的词在各批复核表逐字记录；14 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 骑行：骑行、行程的行 xing；调整车把、调高座垫、前拨调节的调 tiao；车轮、轴承、气嘴、碟刹逐字复核。
- 水上：划水、划船的划 hua；泳池的池 chi，脚蹼的蹼 pu，夹腿板的夹 jia，漂浮的漂 piao；运动时长的长 chang。
- 钟表首饰：调时、调日期、调圈口的调 tiao；走时误差的差 cha；表冠的冠 guan，表壳的壳 ke，圈口的圈 quan；珍珠穿线与归还用本字语境。
- 会展：舞台调度的调 diao，调音与调整的调 tiao；校对稿件的校 jiao；折页的折 zhe，折叠的折 zhe；结构助词的 de。
- 车锁钥匙的钥 yao（不取 yue）；学校泳池的校 xiao，家长的长 zhang；表带褪色、首饰褪色的色 se（不取 shai）。不确定的镶爪长句整句略去，不猜读音。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1103/1103 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，228008 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round23_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.96% → 85.96% | +0.00 | 98.09% → 98.09% |
| 逐词输入·无前文 | 20626 | 77.97% → 77.97% | +0.00 | 97.17% → 97.17% |
| 留出整句 | 4126 | 58.05% → 58.05% | +0.00 | 73.44% → 73.44% |
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
- `docs/lexicon-keep-adding-round23.md`
- `resources/dictionary/expansion/batch342_authored_draft.txt`
- `resources/dictionary/expansion/batch342_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch343_authored_draft.txt`
- `resources/dictionary/expansion/batch343_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch344_authored_draft.txt`
- `resources/dictionary/expansion/batch344_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch345_authored_draft.txt`
- `resources/dictionary/expansion/batch345_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch346_draft.txt`
- `resources/dictionary/expansion/batch346_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch347_draft.txt`
- `resources/dictionary/expansion/batch347_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch348_draft.txt`
- `resources/dictionary/expansion/batch348_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch349_draft.txt`
- `resources/dictionary/expansion/batch349_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_342.md`
- `resources/dictionary/expansion/batch_343.md`
- `resources/dictionary/expansion/batch_344.md`
- `resources/dictionary/expansion/batch_345.md`
- `resources/dictionary/expansion/batch_346.md`
- `resources/dictionary/expansion/batch_347.md`
- `resources/dictionary/expansion/batch_348.md`
- `resources/dictionary/expansion/batch_349.md`
- `resources/dictionary/expansion/lexicon_batch_342.applied.json`
- `resources/dictionary/expansion/lexicon_batch_342.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_342.json`
- `resources/dictionary/expansion/lexicon_batch_342.tsv`
- `resources/dictionary/expansion/lexicon_batch_342.validation.json`
- `resources/dictionary/expansion/lexicon_batch_343.applied.json`
- `resources/dictionary/expansion/lexicon_batch_343.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_343.json`
- `resources/dictionary/expansion/lexicon_batch_343.tsv`
- `resources/dictionary/expansion/lexicon_batch_343.validation.json`
- `resources/dictionary/expansion/lexicon_batch_344.applied.json`
- `resources/dictionary/expansion/lexicon_batch_344.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_344.json`
- `resources/dictionary/expansion/lexicon_batch_344.tsv`
- `resources/dictionary/expansion/lexicon_batch_344.validation.json`
- `resources/dictionary/expansion/lexicon_batch_345.applied.json`
- `resources/dictionary/expansion/lexicon_batch_345.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_345.json`
- `resources/dictionary/expansion/lexicon_batch_345.tsv`
- `resources/dictionary/expansion/lexicon_batch_345.validation.json`
- `resources/dictionary/expansion/lexicon_batch_346.json`
- `resources/dictionary/expansion/lexicon_batch_346.tsv`
- `resources/dictionary/expansion/lexicon_batch_347.json`
- `resources/dictionary/expansion/lexicon_batch_347.tsv`
- `resources/dictionary/expansion/lexicon_batch_348.json`
- `resources/dictionary/expansion/lexicon_batch_348.tsv`
- `resources/dictionary/expansion/lexicon_batch_349.json`
- `resources/dictionary/expansion/lexicon_batch_349.tsv`
- `resources/dictionary/expansion/probes/batch342_probe.tsv`
- `resources/dictionary/expansion/probes/batch343_probe.tsv`
- `resources/dictionary/expansion/probes/batch344_probe.tsv`
- `resources/dictionary/expansion/probes/batch345_probe.tsv`
- `resources/dictionary/expansion/probes/batch346_probe.tsv`
- `resources/dictionary/expansion/probes/batch347_probe.tsv`
- `resources/dictionary/expansion/probes/batch348_probe.tsv`
- `resources/dictionary/expansion/probes/batch349_probe.tsv`
- `resources/dictionary/expansion/round23_exclusions.tsv`
- `resources/dictionary/expansion/round23_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
