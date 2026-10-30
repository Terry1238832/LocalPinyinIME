# 词库续加第54轮（批次 544–549）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `d02f98113cb6ee0267d959cdad63b5b806a581e9`，引擎版本 1.8.1。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：259107 → 260270，新增 **1163 条**。
- 权重 6000 / 5500 / 5000：0 / 1163 / 0 条。
- 最终 SHA-256：`E6F62D9F17CB67867A880B09A33BE76C8CBAC37FCFD36E26B26AE0E907CC185D`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮细分网络联系不畅、临时请假与到岗变动、楼门出入及公共区域联络，均收聊天中可独立输入的短语和常见整句。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 544 | 网络联系不畅与消息延迟聊天 | curate | 416 | 0 | 416 | 0 | 86.06% | 100.00% | `a225edce457ac187a7ecabf12bcb720a5596475b` |
| 545 | 临时请假与到岗变动聊天 | curate | 373 | 0 | 373 | 0 | 75.60% | 100.00% | `a19c113d6c97bddd864fd871cf0536224d245bbb` |
| 546 | 楼门出入与公共区域联络聊天 | curate | 344 | 0 | 344 | 0 | 81.69% | 100.00% | `ac672caba88275df6a275e0bd27ffb395691c585` |
| 547 | 网络联系不畅与消息延迟较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 90.00% | `4ee8cdf738d6a89cbba891814a6c38598fb3770d` |
| 548 | 临时请假与到岗变动较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 100.00% | `6667bc4d39a950487ba7efe09967586f852cb1e2` |
| 549 | 楼门出入与公共区域联络较长说法 | merge | 10 | 0 | 10 | 0 | 80.00% | 90.00% | `0a62c5040eee79f983e2d69d518bcc7528c08afb` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round54_exclusions.tsv`，共 322 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：147 条。
- `outside_batch_length_scope`：171 条。
- `quality: ungrammatical wording`：2 条。
- `quality: unnatural wording`：1 条。
- `duplicate:batch544`：1 条。



同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1023 个含多音字的词在各批复核表逐字记录；25 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

只选常用、语义完整的日常说法；无机械排列组合，不依据测试用例选词。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 75/75 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，260270 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round54_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 88.21% → 88.21% | +0.00 | 98.63% → 98.63% |
| 逐词输入·无前文 | 20626 | 79.64% → 79.64% | +0.00 | 97.53% → 97.53% |
| 留出整句 | 4126 | 64.49% → 64.49% | +0.00 | 77.58% → 77.58% |
| 口语对话·整句 | 3368 | 84.80% → 84.80% | +0.00 | 94.12% → 94.12% |
| 口语对话·逐词 | 19504 | 95.49% → 95.49% | +0.00 | 99.62% → 99.62% |
| 项目留出 | 138 | 84.78% → 84.78% | +0.00 | 89.13% → 89.13% |
| 新增用例 | 67 | 71.64% → 71.64% | +0.00 | 83.58% → 83.58% |
| 独立用例 | 62 | 53.23% → 53.23% | +0.00 | 79.03% → 79.03% |
| 日常口语·开发 | 149 | 97.32% → 97.32% | +0.00 | 99.33% → 99.33% |
| 日常口语·留出 | 150 | 90.00% → 90.00% | +0.00 | 96.67% → 96.67% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round54.md`
- `resources/dictionary/expansion/batch544_authored_draft.txt`
- `resources/dictionary/expansion/batch544_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch545_authored_draft.txt`
- `resources/dictionary/expansion/batch545_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch546_authored_draft.txt`
- `resources/dictionary/expansion/batch546_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch547_draft.txt`
- `resources/dictionary/expansion/batch547_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch548_draft.txt`
- `resources/dictionary/expansion/batch548_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch549_draft.txt`
- `resources/dictionary/expansion/batch549_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_544.md`
- `resources/dictionary/expansion/batch_545.md`
- `resources/dictionary/expansion/batch_546.md`
- `resources/dictionary/expansion/batch_547.md`
- `resources/dictionary/expansion/batch_548.md`
- `resources/dictionary/expansion/batch_549.md`
- `resources/dictionary/expansion/lexicon_batch_544.applied.json`
- `resources/dictionary/expansion/lexicon_batch_544.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_544.json`
- `resources/dictionary/expansion/lexicon_batch_544.tsv`
- `resources/dictionary/expansion/lexicon_batch_544.validation.json`
- `resources/dictionary/expansion/lexicon_batch_545.applied.json`
- `resources/dictionary/expansion/lexicon_batch_545.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_545.json`
- `resources/dictionary/expansion/lexicon_batch_545.tsv`
- `resources/dictionary/expansion/lexicon_batch_545.validation.json`
- `resources/dictionary/expansion/lexicon_batch_546.applied.json`
- `resources/dictionary/expansion/lexicon_batch_546.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_546.json`
- `resources/dictionary/expansion/lexicon_batch_546.tsv`
- `resources/dictionary/expansion/lexicon_batch_546.validation.json`
- `resources/dictionary/expansion/lexicon_batch_547.json`
- `resources/dictionary/expansion/lexicon_batch_547.tsv`
- `resources/dictionary/expansion/lexicon_batch_548.json`
- `resources/dictionary/expansion/lexicon_batch_548.tsv`
- `resources/dictionary/expansion/lexicon_batch_549.json`
- `resources/dictionary/expansion/lexicon_batch_549.tsv`
- `resources/dictionary/expansion/probes/batch544_probe.tsv`
- `resources/dictionary/expansion/probes/batch545_probe.tsv`
- `resources/dictionary/expansion/probes/batch546_probe.tsv`
- `resources/dictionary/expansion/probes/batch547_probe.tsv`
- `resources/dictionary/expansion/probes/batch548_probe.tsv`
- `resources/dictionary/expansion/probes/batch549_probe.tsv`
- `resources/dictionary/expansion/round54_exclusions.tsv`
- `resources/dictionary/expansion/round54_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
