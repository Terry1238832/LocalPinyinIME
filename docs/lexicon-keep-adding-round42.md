# 词库续加第42轮（批次 472–477）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `64f6a1760ddf448830b41079539553ef0ee93894`，引擎版本 1.6.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：246105 → 247124，新增 **1019 条**。
- 权重 6000 / 5500 / 5000：0 / 1019 / 0 条。
- 最终 SHA-256：`0D36991FB654A678F6C56C85A95D96FD695F8746E80898057EDC3BB2346F11FE`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：日常聊天口语。逐条自拟日常钱款分摊与小额往来、轻微身体感受与照应、随手找物与借还提醒三个专题，各一短一长共六批。按既有词库层、domains和本轮前批去重；只按生活常识写常见整段问答，不穷举人称、数字和句尾，不从测试数据或失败用例取词。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 472 | 钱款分摊与小额往来聊天 | curate | 325 | 0 | 325 | 0 | 62.15% | 100.00% | `9aaed13b826b773d172666a900e6fa27feb8f65d` |
| 473 | 身体小感受与日常照应聊天 | curate | 322 | 0 | 322 | 0 | 68.94% | 100.00% | `d1ea5398bc51fb7ac17a877512d05aff181d004c` |
| 474 | 随手找物与借还提醒聊天 | curate | 311 | 0 | 311 | 0 | 69.13% | 100.00% | `6e0cae15c18ccccfddf204ffb339b7aa68802ab3` |
| 475 | 钱款分摊与小额往来较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `3c7bd1bd29e18463881860c4616d284875908cf2` |
| 476 | 身体小感受与日常照应较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `713dad11a1ca10b383663bf7b069a05b2dd7cfe5` |
| 477 | 随手找物与借还提醒较长说法 | merge | 21 | 0 | 21 | 0 | 80.95% | 100.00% | `0fd3dd3475938b3c21b434831e9c9271f4279787` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round42_exclusions.tsv`，共 427 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：19 条。
- `outside_batch_length_scope`：358 条。
- `quality: vague object or awkward whole-chat financial wording`：7 条。
- `quality: unclear payment referent, clipped sharing expression or less natural settlement wording`：7 条。
- `quality: unclear physical referent, clipped body description or less natural whole-chat phrasing`：11 条。
- `quality: clipped body reference, unclear symptom wording or unnecessary reassurance variant`：6 条。
- `quality: clipped placement reminder, weakly natural direction wording or unclear demonstrative reference`：7 条。
- `quality: incomplete object, forced memory wording or weakly natural borrowing phrase`：9 条。
- `quality: unclear protective-cover reference; author a clear shell reference instead`：2 条。
- `quality: unclear reminder recipient; author a direct reminder to me instead`：1 条。

去掉与全部既有层、domains及本轮先前批次重复的词面，并筛除不自然的款项指代、残缺问题、模糊身体描述、强行压短的找物提醒等。保留自然口语的省略，剔除难作为整段输入的弱句；不机械替换人称、金额数字或句尾。噎着一句重新拟成主体清楚的我有点噎着了，保护壳和出门提醒分别明确对象与接受提醒的人；原稿逐条记录。别走了才想起改拟完整的别走了才想起来，进入长词批，不把七字说法截短。三个专题各一短一长共六批，逐条自拟，没有第三方词库导入或测试数据取词。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

934 个含多音字的词在各批复核表逐字记录；40 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐词逐字复核：还钱、还借物、还不上、该还的取huan；仍然和还未取hai；下回还得问你分别取hai/dei。必须义的得取dei，补语及免得、记得的得取de。付重了与重影的重取chong，重物的重取zhong。查账、查查取cha；没收款表示尚未收取，没取mei，不混同没收财物的mo。你帮我省了事的了取le，不按了事liao shi套词；借给谁了得翻记录的了得分别取le/dei，不按了得liao de套词。找根长一点的长取chang，外壳、手机壳的壳取ke。慢点嚼取jiao；睡不着、噎着、饿着、冷着的着取zhao，靠着、盖着、拿着及急着还钱的着取zhe；你别跟着着急两个着分别取zhe/zhao。调风取tiao；热水、干衣物等按常用语境逐字选音，擤鼻涕取xing，哈欠取ha qian；轻声按本字写。谁字出现的候选选择shui，不另增同词面异读。üe统一写lve/nve。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1/1 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，247124 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round42_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round42.md`
- `resources/dictionary/expansion/batch472_authored_draft.txt`
- `resources/dictionary/expansion/batch472_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch473_authored_draft.txt`
- `resources/dictionary/expansion/batch473_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch474_authored_draft.txt`
- `resources/dictionary/expansion/batch474_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch475_draft.txt`
- `resources/dictionary/expansion/batch475_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch476_draft.txt`
- `resources/dictionary/expansion/batch476_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch477_draft.txt`
- `resources/dictionary/expansion/batch477_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_472.md`
- `resources/dictionary/expansion/batch_473.md`
- `resources/dictionary/expansion/batch_474.md`
- `resources/dictionary/expansion/batch_475.md`
- `resources/dictionary/expansion/batch_476.md`
- `resources/dictionary/expansion/batch_477.md`
- `resources/dictionary/expansion/lexicon_batch_472.applied.json`
- `resources/dictionary/expansion/lexicon_batch_472.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_472.json`
- `resources/dictionary/expansion/lexicon_batch_472.tsv`
- `resources/dictionary/expansion/lexicon_batch_472.validation.json`
- `resources/dictionary/expansion/lexicon_batch_473.applied.json`
- `resources/dictionary/expansion/lexicon_batch_473.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_473.json`
- `resources/dictionary/expansion/lexicon_batch_473.tsv`
- `resources/dictionary/expansion/lexicon_batch_473.validation.json`
- `resources/dictionary/expansion/lexicon_batch_474.applied.json`
- `resources/dictionary/expansion/lexicon_batch_474.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_474.json`
- `resources/dictionary/expansion/lexicon_batch_474.tsv`
- `resources/dictionary/expansion/lexicon_batch_474.validation.json`
- `resources/dictionary/expansion/lexicon_batch_475.json`
- `resources/dictionary/expansion/lexicon_batch_475.tsv`
- `resources/dictionary/expansion/lexicon_batch_476.json`
- `resources/dictionary/expansion/lexicon_batch_476.tsv`
- `resources/dictionary/expansion/lexicon_batch_477.json`
- `resources/dictionary/expansion/lexicon_batch_477.tsv`
- `resources/dictionary/expansion/probes/batch472_probe.tsv`
- `resources/dictionary/expansion/probes/batch473_probe.tsv`
- `resources/dictionary/expansion/probes/batch474_probe.tsv`
- `resources/dictionary/expansion/probes/batch475_probe.tsv`
- `resources/dictionary/expansion/probes/batch476_probe.tsv`
- `resources/dictionary/expansion/probes/batch477_probe.tsv`
- `resources/dictionary/expansion/round42_exclusions.tsv`
- `resources/dictionary/expansion/round42_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
