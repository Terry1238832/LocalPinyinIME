# 词库续加第44轮（批次 484–489）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `ac1b60b3dd8eb683cc9ef20cb04bf33729959530`，引擎版本 1.7.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第42轮起累计。

- `local_core`：248233 → 249255，新增 **1022 条**。
- 权重 6000 / 5500 / 5000：0 / 1021 / 1 条。
- 最终 SHA-256：`B33CFFCDF4584FD7702D511AF3E1AF0A588655907EEDF1163F3DF6211EF398E1`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮按聊天口语继续，自拟球局交流、日常拍照分享、到店住店沟通三个方向，每个方向一短一长共六批。只收自然且语义完整的常见交流，去掉含混、机械变体及不够自然的说法；不从测试数据、评测失败或第三方词库取词。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 484 | 球局邀约与场上交流聊天 | curate | 326 | 0 | 326 | 0 | 63.50% | 100.00% | `3e60cf9215d7d913789c1d7bab62c7042b8f63a6` |
| 485 | 日常拍照与相片分享聊天 | curate | 323 | 0 | 323 | 0 | 73.99% | 100.00% | `7cba1af9a896285e2bee697d5ac8b994fa57fafb` |
| 486 | 酒店到店与住店沟通聊天 | curate | 313 | 0 | 312 | 1 | 76.68% | 100.00% | `76d9830f58d7e8df6f332f911d4e9a1e72ab6a17` |
| 487 | 球局邀约与场上交流较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `503c0619267cc07616cc8223deeee0156db856d9` |
| 488 | 日常拍照与相片分享较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 100.00% | `3f679da06daae0d622b2df64457e173056d8595f` |
| 489 | 酒店到店与住店沟通较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 95.00% | `9b57eabaf4ec26fade47e41a4fefa3c1c82c16a8` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round44_exclusions.tsv`，共 357 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `outside_batch_length_scope`：320 条。
- `duplicate:local_core_zh_pinyin.tsv`：16 条。
- `pronunciation: defer mechanical 卡 wording rather than guess`：1 条。
- `quality: clipped field reference, incomplete traction description or unnatural receiving-speed wording`：5 条。
- `quality: clipped photographic referent, unclear hand instruction or weakly natural whole-chat wording`：6 条。
- `pronunciation: replace uncertain isolated 过曝 reading with a clear brightness description`：1 条。
- `quality: incomplete luggage ownership, missing destination or clipped room-light description`：5 条。
- `quality: unclear spare-racket location, clipped court reference or unnatural camera timer wording`：3 条。

原稿球鞋抓地不够、先擦一下地上、我接慢一点、我在这里空着等残缺或不自然表达不收，改拟清楚的擦地、空位与灯坏提醒。拍照草稿删含混指手、腿部与带子指代，改为看我指的这边、带子套手上、原片我留着、脸亮得发白。住店草稿删未说完整的行李归属、电梯去向、房灯描述，改为这两件是我的、我上来开门、这灯有点暗。长句的备用拍子位置、捡球场地和相机倒计时另拟完整说法，旧稿逐条记录。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

921 个含多音字的词在各批复核表逐字记录；21 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐词逐字审阅：还差人、还借拍子与房间还没好用hai，还给前台用huan；重发、重打、重来、重复与重影用chong。必须义得用dei，补语得用de。没收住为未控制住，没用mei，不读没收财物的mo；鞋带松了得重新系一下的了得分属句末和情态，读le/dei，系鞋带读ji。球弹出去、弹得高读tan；摔着读zhao，拿着、看着、打着、挨着用zhe。调球网、调暗、调时间、调空调温度用tiao，空调的调亦tiao。显得腿长、拉长读chang；露个头的口语用lou；卡落房间、别落下球拍用la。照片底片改为更明确的原片，含混机械卡及孤立过曝原稿不收。轻声按本字，üe写lve/nve。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。本轮无 2–4 字新词；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，249255 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round44_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 88.23% → 88.23% | +0.00 | 98.65% → 98.65% |
| 逐词输入·无前文 | 20626 | 79.64% → 79.64% | +0.00 | 97.53% → 97.53% |
| 留出整句 | 4126 | 64.44% → 64.44% | +0.00 | 77.70% → 77.70% |
| 口语对话·整句 | 3368 | 84.80% → 84.80% | +0.00 | 94.24% → 94.24% |
| 口语对话·逐词 | 19504 | 95.50% → 95.50% | +0.00 | 99.63% → 99.63% |
| 项目留出 | 138 | 84.78% → 84.78% | +0.00 | 89.13% → 89.13% |
| 新增用例 | 67 | 71.64% → 71.64% | +0.00 | 83.58% → 83.58% |
| 独立用例 | 62 | 53.23% → 53.23% | +0.00 | 79.03% → 79.03% |
| 日常口语·开发 | 149 | 97.32% → 97.32% | +0.00 | 99.33% → 99.33% |
| 日常口语·留出 | 150 | 90.00% → 90.00% | +0.00 | 96.00% → 96.00% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round44.md`
- `resources/dictionary/expansion/batch484_authored_draft.txt`
- `resources/dictionary/expansion/batch484_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch485_authored_draft.txt`
- `resources/dictionary/expansion/batch485_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch486_authored_draft.txt`
- `resources/dictionary/expansion/batch486_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch487_draft.txt`
- `resources/dictionary/expansion/batch487_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch488_draft.txt`
- `resources/dictionary/expansion/batch488_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch489_draft.txt`
- `resources/dictionary/expansion/batch489_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_484.md`
- `resources/dictionary/expansion/batch_485.md`
- `resources/dictionary/expansion/batch_486.md`
- `resources/dictionary/expansion/batch_487.md`
- `resources/dictionary/expansion/batch_488.md`
- `resources/dictionary/expansion/batch_489.md`
- `resources/dictionary/expansion/lexicon_batch_484.applied.json`
- `resources/dictionary/expansion/lexicon_batch_484.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_484.json`
- `resources/dictionary/expansion/lexicon_batch_484.tsv`
- `resources/dictionary/expansion/lexicon_batch_484.validation.json`
- `resources/dictionary/expansion/lexicon_batch_485.applied.json`
- `resources/dictionary/expansion/lexicon_batch_485.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_485.json`
- `resources/dictionary/expansion/lexicon_batch_485.tsv`
- `resources/dictionary/expansion/lexicon_batch_485.validation.json`
- `resources/dictionary/expansion/lexicon_batch_486.applied.json`
- `resources/dictionary/expansion/lexicon_batch_486.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_486.json`
- `resources/dictionary/expansion/lexicon_batch_486.tsv`
- `resources/dictionary/expansion/lexicon_batch_486.validation.json`
- `resources/dictionary/expansion/lexicon_batch_487.json`
- `resources/dictionary/expansion/lexicon_batch_487.tsv`
- `resources/dictionary/expansion/lexicon_batch_488.json`
- `resources/dictionary/expansion/lexicon_batch_488.tsv`
- `resources/dictionary/expansion/lexicon_batch_489.json`
- `resources/dictionary/expansion/lexicon_batch_489.tsv`
- `resources/dictionary/expansion/probes/batch484_probe.tsv`
- `resources/dictionary/expansion/probes/batch485_probe.tsv`
- `resources/dictionary/expansion/probes/batch486_probe.tsv`
- `resources/dictionary/expansion/probes/batch487_probe.tsv`
- `resources/dictionary/expansion/probes/batch488_probe.tsv`
- `resources/dictionary/expansion/probes/batch489_probe.tsv`
- `resources/dictionary/expansion/round44_exclusions.tsv`
- `resources/dictionary/expansion/round44_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
