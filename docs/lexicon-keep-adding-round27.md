# 词库续加第27轮（批次 374–381）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `1837fad140c3b6b98079d62694dd0a4c942c2a9e`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：232171 → 233539，新增 **1368 条**。
- 权重 6000 / 5500 / 5000：0 / 1361 / 7 条。
- 最终 SHA-256：`214270008A919D4DE1362ED7A47E587A4B99B68F37D51AC3D83A304CFCDB536E`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

原主题清单已完成，本轮自拟航空乘机与机场服务、家用照明与灯具选用、观鸟与常见鸟类观察、平面设计与海报制作。对应出行办理手续、家庭灯光选用、日常自然观察、工作学习活动图文制作等会整词输入的场景。与既有旅游票务、家电维修、自然景观、摄影办公主题有相关处，但没有对应专批；逐条全库去重，删去参数排列、冷僻行业词以及人名商标衍生标签。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 374 | 航空乘机与机场服务 | curate | 340 | 0 | 340 | 0 | 81.18% | 100.00% | `14d9a79e73cb2a29481f43b11dec1604324d2d09` |
| 375 | 家用照明与灯具选用 | curate | 301 | 0 | 300 | 1 | 95.02% | 100.00% | `8db661df361e26290a890deac33e86a1e5241762` |
| 376 | 观鸟与常见鸟类观察 | curate | 326 | 0 | 321 | 5 | 81.90% | 100.00% | `853cf0b8be9c19b560aeb8968aa88e0805929fe6` |
| 377 | 平面设计与海报制作 | curate | 321 | 0 | 320 | 1 | 95.02% | 100.00% | `bb7f87fe8867155a9d71ae7b62f2cfab91a00e89` |
| 378 | 航空乘机与机场服务较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `23ccfa6ca03f12e73f8dd2df8b58fa28064b8daa` |
| 379 | 家用照明与灯具选用较长说法 | merge | 20 | 0 | 20 | 0 | 60.00% | 100.00% | `f326f600cd2a3d015beb8b9f7c2a22a190e980d4` |
| 380 | 观鸟与常见鸟类观察较长说法 | merge | 20 | 0 | 20 | 0 | 90.00% | 100.00% | `8e478b171b40dfe697890e137e9274920ee018b9` |
| 381 | 平面设计与海报制作较长说法 | merge | 20 | 0 | 20 | 0 | 45.00% | 100.00% | `f889c158b1e0d0e6c87be6f063158353d3b6f3ad` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round27_exclusions.tsv`，共 1786 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality: unnatural short phrase, redundant label or excessively narrow detail`：261 条。
- `duplicate:local_core_zh_pinyin.tsv`：514 条。
- `quality: repetitive parameter, unnatural phrase or overly narrow aviation detail`：239 条。
- `quality: name-derived label, questionable everyday usage or specialist detail`：53 条。
- `duplicate:standard_zh_pinyin.tsv`：89 条。
- `quality: repetitive lamp parameter, uncommon fixture subtype or uncertain marketing label`：206 条。
- `quality: overly narrow ornithology detail, uncommon species label or repetitive observation phrase`：231 条。
- `probe_top9: zhiji absent from top nine; omit bird term rather than increase priority in crowded everyday homophone group`：1 条。
- `probe_top9: shiji absent from top nine; existing homophone minimum 5500 prevents promotion`：1 条。
- `probe_top9: yiyu absent from top nine; omit less common feather term rather than increase priority over established everyday homophones`：1 条。
- `quality: repetitive design parameter, uncommon print jargon or unnatural phrase`：190 条。

首次探针中石鸡 shiji、翼羽 yiyu、雉鸡 zhiji 未进前九。石鸡受原有同拼最低 5500 档约束；后两词虽可考虑 6000 档，但同拼已有多条更常用说法，本轮选择删去三条，保留较低竞争影响。随后从基线重建未提交批次，重跑双架构、全部探针、curate evaluate、准确率和结构校验。未撤回整批。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1092 个含多音字的词在各批复核表逐字记录；15 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐条阅读全文全拼。行李均取 xingli，不取 hang；还没过安检的没取 mei；灯具调光调色取 tiao，长臂台灯取 chang；长耳鸮、黑翅长脚鹬及尾巴长的长取 chang，鹎取 bei、鸲取 qu、鸊鷉取 pi ti、鸸鹋取 er miao、栖息取 qi，冠羽取 guan；长投影和长图取 chang；排版断行取 hang，字体转曲取 zhuan qu，印刷出血取 xue；灯带脱落的落取 luo。拿不准的格栅细分灯具、冷僻鸟名和工艺术语删除，不以默认音代替判断。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1019/1019 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，233539 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round27_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.95% → 85.95% | +0.00 | 98.09% → 98.09% |
| 逐词输入·无前文 | 20626 | 77.97% → 77.97% | +0.00 | 97.17% → 97.17% |
| 留出整句 | 4126 | 58.05% → 58.05% | +0.00 | 73.44% → 73.44% |
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
- `docs/lexicon-keep-adding-round27.md`
- `resources/dictionary/expansion/batch374_authored_draft.txt`
- `resources/dictionary/expansion/batch374_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch375_authored_draft.txt`
- `resources/dictionary/expansion/batch375_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch376_authored_draft.txt`
- `resources/dictionary/expansion/batch376_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch377_authored_draft.txt`
- `resources/dictionary/expansion/batch377_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch378_draft.txt`
- `resources/dictionary/expansion/batch378_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch379_draft.txt`
- `resources/dictionary/expansion/batch379_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch380_draft.txt`
- `resources/dictionary/expansion/batch380_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch381_draft.txt`
- `resources/dictionary/expansion/batch381_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_374.md`
- `resources/dictionary/expansion/batch_375.md`
- `resources/dictionary/expansion/batch_376.md`
- `resources/dictionary/expansion/batch_377.md`
- `resources/dictionary/expansion/batch_378.md`
- `resources/dictionary/expansion/batch_379.md`
- `resources/dictionary/expansion/batch_380.md`
- `resources/dictionary/expansion/batch_381.md`
- `resources/dictionary/expansion/lexicon_batch_374.applied.json`
- `resources/dictionary/expansion/lexicon_batch_374.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_374.json`
- `resources/dictionary/expansion/lexicon_batch_374.tsv`
- `resources/dictionary/expansion/lexicon_batch_374.validation.json`
- `resources/dictionary/expansion/lexicon_batch_375.applied.json`
- `resources/dictionary/expansion/lexicon_batch_375.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_375.json`
- `resources/dictionary/expansion/lexicon_batch_375.tsv`
- `resources/dictionary/expansion/lexicon_batch_375.validation.json`
- `resources/dictionary/expansion/lexicon_batch_376.applied.json`
- `resources/dictionary/expansion/lexicon_batch_376.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_376.json`
- `resources/dictionary/expansion/lexicon_batch_376.tsv`
- `resources/dictionary/expansion/lexicon_batch_376.validation.json`
- `resources/dictionary/expansion/lexicon_batch_377.applied.json`
- `resources/dictionary/expansion/lexicon_batch_377.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_377.json`
- `resources/dictionary/expansion/lexicon_batch_377.tsv`
- `resources/dictionary/expansion/lexicon_batch_377.validation.json`
- `resources/dictionary/expansion/lexicon_batch_378.json`
- `resources/dictionary/expansion/lexicon_batch_378.tsv`
- `resources/dictionary/expansion/lexicon_batch_379.json`
- `resources/dictionary/expansion/lexicon_batch_379.tsv`
- `resources/dictionary/expansion/lexicon_batch_380.json`
- `resources/dictionary/expansion/lexicon_batch_380.tsv`
- `resources/dictionary/expansion/lexicon_batch_381.json`
- `resources/dictionary/expansion/lexicon_batch_381.tsv`
- `resources/dictionary/expansion/probes/batch374_probe.tsv`
- `resources/dictionary/expansion/probes/batch375_probe.tsv`
- `resources/dictionary/expansion/probes/batch376_probe.tsv`
- `resources/dictionary/expansion/probes/batch377_probe.tsv`
- `resources/dictionary/expansion/probes/batch378_probe.tsv`
- `resources/dictionary/expansion/probes/batch379_probe.tsv`
- `resources/dictionary/expansion/probes/batch380_probe.tsv`
- `resources/dictionary/expansion/probes/batch381_probe.tsv`
- `resources/dictionary/expansion/round27_exclusions.tsv`
- `resources/dictionary/expansion/round27_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
