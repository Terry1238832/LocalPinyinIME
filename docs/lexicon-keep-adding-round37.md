# 词库续加第37轮（批次 436–443）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `9df03bb1dd89c1c022022065b6199912ab4d723d`，引擎版本 1.6.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：239525 → 240935，新增 **1410 条**。
- 权重 6000 / 5500 / 5000：0 / 1410 / 0 条。
- 最终 SHA-256：`824DF6848A2BACE965B82DD9F1B19D5B7F7FCA3DAAD50A18D862F7085F1D9F0C`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：日常聊天口语。继续逐条自拟途中上下车接送、买东西取货换退、近况问候分享、小失误及补救四类会整段输入的常用说法；侧重具体口语搭配和语境清晰的问答，不从评测案例选词，不批量替换人物、数字、日期或句尾凑数。与第一阶段同领域的名词术语表区别在于本轮是聊天表达；仍按所有既有词面和本轮先前批次去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 436 | 途中上下车与接送聊天 | curate | 343 | 0 | 343 | 0 | 72.59% | 100.00% | `e2d5e0a6b72b0be1a9bfe67e931845575a022d14` |
| 437 | 买东西取货与换退聊天 | curate | 352 | 0 | 352 | 0 | 75.00% | 100.00% | `688be6dee267e8510e96bb6cb656c517c1d9bfae` |
| 438 | 近况问候与随口分享聊天 | curate | 333 | 0 | 333 | 0 | 75.38% | 100.00% | `58684ab47ad5118ccb3a38241d9243c9c7d4e641` |
| 439 | 小失误与事后补救聊天 | curate | 302 | 0 | 302 | 0 | 75.50% | 100.00% | `2e281508ecb04764aa191e777dd675065402c7f6` |
| 440 | 途中上下车与接送较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `38d21138943c74f27e1afabbbf4e4dba3582091b` |
| 441 | 买东西取货与换退较长说法 | merge | 20 | 0 | 20 | 0 | 90.00% | 100.00% | `df7ea9de6c144a2041579c0c27f62837ff559ae4` |
| 442 | 近况问候与随口分享较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 100.00% | `78ca26eda8cf45d341d28e614cca378bf70415f3` |
| 443 | 小失误与事后补救较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `ddee504ebdd98b444b4fe2ac940934df2e8b63cf` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round37_exclusions.tsv`，共 259 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：25 条。
- `quality: incomplete or weakly idiomatic travel reply`：11 条。
- `pronunciation: mechanical 卡 wording not confidently resolved in this context; defer instead of using the default reading`：4 条。
- `quality: incomplete clause, forced word order or insufficiently natural whole-chat wording`：32 条。
- `quality: uncommon modern chat wording, truncated object or forced conversational syntax`：13 条。
- `outside_batch_length_scope`：148 条。
- `quality: truncated family reference or forced inversion in casual chat`：7 条。
- `quality: incomplete recollection, stilted reminder or weakly natural correction phrase`：11 条。
- `quality: clipped repair wording, unclear reminder or stacked sentence particles`：8 条。

按全层词面去重；剔除残缺的回忆和时间分句、不自然的维修缩略说法、堆叠语气词及不够明确的倒装。四条含机械卡的候选因字音把握不足而排除，不使用默认拼读。其他默认读音与上下文不符的词在冻结前逐词覆盖，完整表见各批读音复核与批次JSON；没有从测试数据或评测失败答案选词。 原生验收首次x64的test_tsf_integration未通过，保留原日志哈希；在全量准确率作业结束后，单独重新运行x64/x86完整构建及ctest。没有修改引擎或测试来规避失败，最终结果见验收表和JSON。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1332 个含多音字的词在各批复核表逐字记录；52 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

归还、退还物件的还取huan，仍然、还没的还取hai；我得、得出去、这不得找你嘛中的得取dei；固定说法不得不以及状态补语的得取de；东西遗落在柜台或座位的落取la；漏看一行的行取hang；买重了表示重复取chong，重做也取chong，轻重的重取zhong；眯着眼、等着取zhe，找不着、逮着、睡不着取zhao；调休的调取tiao；这儿有点勒取lei；跟标的不一样的的取de，补给你钱的给取gei，均避免误套标的、补给等另一语境的默认词组读音。四条机械卡的候选因未能可靠确认当前语境读音而暂不收；字音复核仅参考[教育部卡字释读](https://dict.revised.moe.edu.tw/dictView.jsp?ID=6336&la=1&powerMode=0)，原候选已先自拟，没有从辞典页面或例句取词，未导入第三方词表。轻声按本字写，üe写lve/nve。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1/1 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，240935 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round37_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round37.md`
- `resources/dictionary/expansion/batch436_authored_draft.txt`
- `resources/dictionary/expansion/batch436_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch437_authored_draft.txt`
- `resources/dictionary/expansion/batch437_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch438_authored_draft.txt`
- `resources/dictionary/expansion/batch438_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch439_authored_draft.txt`
- `resources/dictionary/expansion/batch439_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch440_draft.txt`
- `resources/dictionary/expansion/batch440_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch441_draft.txt`
- `resources/dictionary/expansion/batch441_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch442_draft.txt`
- `resources/dictionary/expansion/batch442_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch443_draft.txt`
- `resources/dictionary/expansion/batch443_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_436.md`
- `resources/dictionary/expansion/batch_437.md`
- `resources/dictionary/expansion/batch_438.md`
- `resources/dictionary/expansion/batch_439.md`
- `resources/dictionary/expansion/batch_440.md`
- `resources/dictionary/expansion/batch_441.md`
- `resources/dictionary/expansion/batch_442.md`
- `resources/dictionary/expansion/batch_443.md`
- `resources/dictionary/expansion/lexicon_batch_436.applied.json`
- `resources/dictionary/expansion/lexicon_batch_436.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_436.json`
- `resources/dictionary/expansion/lexicon_batch_436.tsv`
- `resources/dictionary/expansion/lexicon_batch_436.validation.json`
- `resources/dictionary/expansion/lexicon_batch_437.applied.json`
- `resources/dictionary/expansion/lexicon_batch_437.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_437.json`
- `resources/dictionary/expansion/lexicon_batch_437.tsv`
- `resources/dictionary/expansion/lexicon_batch_437.validation.json`
- `resources/dictionary/expansion/lexicon_batch_438.applied.json`
- `resources/dictionary/expansion/lexicon_batch_438.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_438.json`
- `resources/dictionary/expansion/lexicon_batch_438.tsv`
- `resources/dictionary/expansion/lexicon_batch_438.validation.json`
- `resources/dictionary/expansion/lexicon_batch_439.applied.json`
- `resources/dictionary/expansion/lexicon_batch_439.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_439.json`
- `resources/dictionary/expansion/lexicon_batch_439.tsv`
- `resources/dictionary/expansion/lexicon_batch_439.validation.json`
- `resources/dictionary/expansion/lexicon_batch_440.json`
- `resources/dictionary/expansion/lexicon_batch_440.tsv`
- `resources/dictionary/expansion/lexicon_batch_441.json`
- `resources/dictionary/expansion/lexicon_batch_441.tsv`
- `resources/dictionary/expansion/lexicon_batch_442.json`
- `resources/dictionary/expansion/lexicon_batch_442.tsv`
- `resources/dictionary/expansion/lexicon_batch_443.json`
- `resources/dictionary/expansion/lexicon_batch_443.tsv`
- `resources/dictionary/expansion/probes/batch436_probe.tsv`
- `resources/dictionary/expansion/probes/batch437_probe.tsv`
- `resources/dictionary/expansion/probes/batch438_probe.tsv`
- `resources/dictionary/expansion/probes/batch439_probe.tsv`
- `resources/dictionary/expansion/probes/batch440_probe.tsv`
- `resources/dictionary/expansion/probes/batch441_probe.tsv`
- `resources/dictionary/expansion/probes/batch442_probe.tsv`
- `resources/dictionary/expansion/probes/batch443_probe.tsv`
- `resources/dictionary/expansion/round37_exclusions.tsv`
- `resources/dictionary/expansion/round37_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
