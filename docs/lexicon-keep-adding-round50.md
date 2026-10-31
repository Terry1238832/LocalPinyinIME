# 词库续加第50轮（批次 520–525）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `5eb6f95ccb0f015d99c4e0e1028575d163c08da4`，引擎版本 1.7.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第42轮起累计。

- `local_core`：254823 → 255869，新增 **1046 条**。
- 权重 6000 / 5500 / 5000：0 / 1046 / 0 条。
- 最终 SHA-256：`E798551D4F16491695BE7FFA425776B89492839F710A714BCC2EAABDB2A5E2BD`。
- CMake 上限从 255390 调整到 281456，约为新总数的 110%；独立提交 `1b645d2da0a18cad496bc8675ccfbbd4b4b030b4`。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

搬家打包和临时搭手、阳台养花照料、周末野餐组织都是常见生活交流场景；短批收自然、可独立输入的聊天表达，长批收完整且常用的协调句。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 520 | 搬家打包与搬运搭手聊天 | curate | 344 | 0 | 344 | 0 | 61.05% | 100.00% | `6013c691949fc433e47fef1d7a7b8f8cba5cee04` |
| 521 | 阳台养花与植物照料聊天 | curate | 353 | 0 | 353 | 0 | 63.46% | 100.00% | `4db95fc876a13868ac9f3d340214ede4d88812ee` |
| 522 | 周末野餐与户外聚会聊天 | curate | 319 | 0 | 319 | 0 | 70.85% | 100.00% | `ad835c48defd7f181f4f5bcc2442bfee5ddfa7a8` |
| 523 | 搬家打包与搬运较长说法 | merge | 10 | 0 | 10 | 0 | 60.00% | 70.00% | `86730770ecbc94d2f4289a8afd15836984a9dcd7` |
| 524 | 阳台养花与植物照料较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 90.00% | `5b88328316820662c6a5f89f4befa89971ea6015` |
| 525 | 周末野餐与户外聚会较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 100.00% | `4fda029009fa596aa8f8ee87b47d426ddf3561ae` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round50_exclusions.tsv`，共 95 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：47 条。
- `outside_batch_length_scope`：9 条。
- `duplicate:batch521`：3 条。
- `duplicate:batch520`：1 条。
- `quality: clipped object, awkward wording or narrow fragment; replaced with a complete natural chat form where useful`：32 条。
- `quality: specialized or unnatural plant phrase; use a common conversational wording instead`：2 条。
- `quality: current informal trend wording; prefer a stable everyday photo comment`：1 条。



同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

925 个含多音字的词在各批复核表逐字记录；15 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

搬运语境中重物的重读 zhong，侧着搬的着读 zhe；还未/还有/仍然的还读 hai，交回卡的还读 huan；行李、行不行及就行均依语境读 xing。养花语境中生长、长短、长椅的长读 chang；种子/留种/播种中的种读 zhong，动词种植读 zhong；暖和的和读 huo，连接词和读 he；结籽读 jie。倒水、倒空的倒读 dao，着急读 zhao、持续状态的着读 zhe；空调及空闲为空 kong，调料 tiao，位置处 chu，落山/落在读 luo，薄薄一层 bao，多少/少量 shao，发送/发芽/fa 黄中的发读 fa。补语得读 de，必须义得读 dei。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 9/9 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，255869 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round50_validation.json`；短词批次另存工具生成的 `.validation.json`。
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

- `CMakeLists.txt`
- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round50.md`
- `resources/dictionary/expansion/batch520_authored_draft.txt`
- `resources/dictionary/expansion/batch520_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch521_authored_draft.txt`
- `resources/dictionary/expansion/batch521_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch522_authored_draft.txt`
- `resources/dictionary/expansion/batch522_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch523_draft.txt`
- `resources/dictionary/expansion/batch523_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch524_draft.txt`
- `resources/dictionary/expansion/batch524_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch525_draft.txt`
- `resources/dictionary/expansion/batch525_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_520.md`
- `resources/dictionary/expansion/batch_521.md`
- `resources/dictionary/expansion/batch_522.md`
- `resources/dictionary/expansion/batch_523.md`
- `resources/dictionary/expansion/batch_524.md`
- `resources/dictionary/expansion/batch_525.md`
- `resources/dictionary/expansion/lexicon_batch_520.applied.json`
- `resources/dictionary/expansion/lexicon_batch_520.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_520.json`
- `resources/dictionary/expansion/lexicon_batch_520.tsv`
- `resources/dictionary/expansion/lexicon_batch_520.validation.json`
- `resources/dictionary/expansion/lexicon_batch_521.applied.json`
- `resources/dictionary/expansion/lexicon_batch_521.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_521.json`
- `resources/dictionary/expansion/lexicon_batch_521.tsv`
- `resources/dictionary/expansion/lexicon_batch_521.validation.json`
- `resources/dictionary/expansion/lexicon_batch_522.applied.json`
- `resources/dictionary/expansion/lexicon_batch_522.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_522.json`
- `resources/dictionary/expansion/lexicon_batch_522.tsv`
- `resources/dictionary/expansion/lexicon_batch_522.validation.json`
- `resources/dictionary/expansion/lexicon_batch_523.json`
- `resources/dictionary/expansion/lexicon_batch_523.tsv`
- `resources/dictionary/expansion/lexicon_batch_524.json`
- `resources/dictionary/expansion/lexicon_batch_524.tsv`
- `resources/dictionary/expansion/lexicon_batch_525.json`
- `resources/dictionary/expansion/lexicon_batch_525.tsv`
- `resources/dictionary/expansion/probes/batch520_probe.tsv`
- `resources/dictionary/expansion/probes/batch521_probe.tsv`
- `resources/dictionary/expansion/probes/batch522_probe.tsv`
- `resources/dictionary/expansion/probes/batch523_probe.tsv`
- `resources/dictionary/expansion/probes/batch524_probe.tsv`
- `resources/dictionary/expansion/probes/batch525_probe.tsv`
- `resources/dictionary/expansion/round50_exclusions.tsv`
- `resources/dictionary/expansion/round50_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
