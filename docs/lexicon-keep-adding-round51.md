# 词库续加第51轮（批次 526–531）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `a6fede60adad025288d6e7fc5b877e4b3aa54d38`，引擎版本 1.8.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：255869 → 256888，新增 **1019 条**。
- 权重 6000 / 5500 / 5000：0 / 1019 / 0 条。
- 最终 SHA-256：`28FC5156C70D8CE656DAE4D02B0E8ADDE367B90C6FB7DADC144A7109395A12E5`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

菜市场买菜、水电燃气缴费报修和生日聚会准备是常见生活交流；短批聚焦真实问答、选择、分工与招呼，长批是完整协调说法。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 526 | 菜市场买菜挑选与问价聊天 | curate | 319 | 0 | 319 | 0 | 64.58% | 100.00% | `7c4cd98e5b94a60dc3433b27a7012220e5d32682` |
| 527 | 家庭水电燃气缴费与临时报修聊天 | curate | 315 | 0 | 315 | 0 | 81.90% | 100.00% | `6c71ec5e32794e0e3f36722ea69e92cfe5dedba8` |
| 528 | 生日聚会准备与到场招呼聊天 | curate | 354 | 0 | 354 | 0 | 80.79% | 100.00% | `c4199af67af054b80cb135e5b53a25b25bd02a08` |
| 529 | 菜市场买菜挑选与问价较长说法 | merge | 11 | 0 | 11 | 0 | 90.91% | 90.91% | `9ee6d83e414cb259dc5d3467807e75da607c8555` |
| 530 | 家庭水电燃气缴费与临时报修较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 90.00% | `b0fa40dc5fd90a3ad40dfabe951f333dc33a0bf3` |
| 531 | 生日聚会准备与到场招呼较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 90.00% | `4eee3d288bdbfd223d0dc81094176d0e240bcb0c` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round51_exclusions.tsv`，共 166 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：117 条。
- `outside_batch_length_scope`：13 条。
- `quality: clipped phrase; replaced with 今早才进货`：1 条。
- `quality: awkward instruction; replaced with 肉先包好吧`：1 条。
- `quality: clipped phrase; replaced with 这摊队真长`：1 条。
- `quality: unclear phrase; replaced with 这摊刚开张`：1 条。
- `quality: unnatural wording; replaced with 焦糊味道`：2 条。
- `quality: specialist fragment not natural chat`：2 条。
- `quality: technical fragment not natural chat`：2 条。
- `quality: awkward wording; replaced with 热水器不热`：1 条。
- `quality: imprecise clipped phrase; replaced with 水电费单在哪`：1 条。
- `duplicate:standard_zh_pinyin.tsv`：3 条。
- `quality: clipped phrase`：2 条。
- `quality: missing 是; replaced with 今天是你生日`：1 条。
- `quality: unnatural wording; replaced with 周末过生日`：1 条。
- `quality: clipped instruction; replaced with 把灯串打开`：1 条。
- `quality: awkward word order; replaced with 我给你倒茶`：1 条。
- `quality: awkward word order; replaced with 小孩喝果汁`：1 条。
- `quality: clipped phrase; replaced with 寿星吃最大块`：1 条。
- `quality: unclear wording; replaced with 气球先别拆`：1 条。
- `quality: awkward wording; replaced with 回家再拆礼物`：1 条。
- `quality: clipped instruction; replaced with 把音乐关小`：1 条。
- `quality: uncommon term; replaced with 门禁码发我`：1 条。
- `quality: awkward in arrival context; replaced with 路上慢点`：1 条。
- `quality: clipped instruction; replaced with 帮我留车位`：1 条。
- `quality: ungrammatical in this context; replaced with 很喜欢这个`：1 条。
- `quality: unclear instruction; replaced with 一人拆一个`：1 条。
- `quality: awkward word order; replaced with 下次再一起`：1 条。
- `quality: less natural wording; replaced with 到家发个消息`：1 条。
- `quality: semantically reversed; replaced with 让寿星许愿`：1 条。
- `quality: awkward phrase; replaced with 最大块给寿星`：1 条。
- `quality: incomplete phrase; replaced with 下次再聚吧`：1 条。



同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

859 个含多音字的词在各批复核表逐字记录；9 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

价与价格读 jia，便宜读 pianyi、方便读 bianfang；排队长读 chang，长一岁、长高、长大读 zhang。还在、还有、还没、还会等表示仍然或另外时读 hai；着急和点不着读 zhao，表示动作状态的着读 zhe。重秤中的重读 zhong，重新、重复读 chong；调低、调整读 tiao，音乐读 yue，生日快乐读 le。得换、得去等需要义读 dei，补语得读 de。其余称、量、空、倒、处、发、和、少等按本轮具体语境选音，并逐字写入复核表。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 164/164 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，256888 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round51_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round51.md`
- `resources/dictionary/expansion/batch526_authored_draft.txt`
- `resources/dictionary/expansion/batch526_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch527_authored_draft.txt`
- `resources/dictionary/expansion/batch527_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch528_authored_draft.txt`
- `resources/dictionary/expansion/batch528_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch529_draft.txt`
- `resources/dictionary/expansion/batch529_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch530_draft.txt`
- `resources/dictionary/expansion/batch530_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch531_draft.txt`
- `resources/dictionary/expansion/batch531_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_526.md`
- `resources/dictionary/expansion/batch_527.md`
- `resources/dictionary/expansion/batch_528.md`
- `resources/dictionary/expansion/batch_529.md`
- `resources/dictionary/expansion/batch_530.md`
- `resources/dictionary/expansion/batch_531.md`
- `resources/dictionary/expansion/lexicon_batch_526.applied.json`
- `resources/dictionary/expansion/lexicon_batch_526.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_526.json`
- `resources/dictionary/expansion/lexicon_batch_526.tsv`
- `resources/dictionary/expansion/lexicon_batch_526.validation.json`
- `resources/dictionary/expansion/lexicon_batch_527.applied.json`
- `resources/dictionary/expansion/lexicon_batch_527.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_527.json`
- `resources/dictionary/expansion/lexicon_batch_527.tsv`
- `resources/dictionary/expansion/lexicon_batch_527.validation.json`
- `resources/dictionary/expansion/lexicon_batch_528.applied.json`
- `resources/dictionary/expansion/lexicon_batch_528.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_528.json`
- `resources/dictionary/expansion/lexicon_batch_528.tsv`
- `resources/dictionary/expansion/lexicon_batch_528.validation.json`
- `resources/dictionary/expansion/lexicon_batch_529.json`
- `resources/dictionary/expansion/lexicon_batch_529.tsv`
- `resources/dictionary/expansion/lexicon_batch_530.json`
- `resources/dictionary/expansion/lexicon_batch_530.tsv`
- `resources/dictionary/expansion/lexicon_batch_531.json`
- `resources/dictionary/expansion/lexicon_batch_531.tsv`
- `resources/dictionary/expansion/probes/batch526_probe.tsv`
- `resources/dictionary/expansion/probes/batch527_probe.tsv`
- `resources/dictionary/expansion/probes/batch528_probe.tsv`
- `resources/dictionary/expansion/probes/batch529_probe.tsv`
- `resources/dictionary/expansion/probes/batch530_probe.tsv`
- `resources/dictionary/expansion/probes/batch531_probe.tsv`
- `resources/dictionary/expansion/round51_exclusions.tsv`
- `resources/dictionary/expansion/round51_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
