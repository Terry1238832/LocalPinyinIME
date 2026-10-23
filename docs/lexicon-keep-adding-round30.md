# 词库续加第30轮（批次 398–403）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `74dcdfbd301f87d1b7d7b9bebba3a0d65eb0222b`，引擎版本 1.2.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：235534 → 236017，新增 **483 条**。
- 权重 6000 / 5500 / 5000：0 / 483 / 0 条。
- 最终 SHA-256：`FF298629BD2DC04197F3682BC2CEFC2B4D04D5FE44BDB74AEAB071517A1F4AA4`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：B。按第二阶段说法类型顺序补客套话与礼貌用语、道歉和感谢、祝福语与日常问候三类。每条先按自然整词输入习惯拟写，再与所有层和场景词表去重；不做称呼、主语、程度副词的笛卡尔积，不用测评缺词作选词依据。短词每批 100–400 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 398 | 客套话与礼貌用语补漏 | curate | 157 | 0 | 157 | 0 | 98.09% | 100.00% | `04a61e5e921307f28a4e026902647b3ac0dc8a40` |
| 399 | 道歉与感谢表达补漏 | curate | 156 | 0 | 156 | 0 | 87.18% | 100.00% | `d7dbfe9a745e99509acdc7df374b84d8417e51c6` |
| 400 | 祝福语与日常问候补漏 | curate | 140 | 0 | 140 | 0 | 88.57% | 100.00% | `cf30c1081c6b65a87e463eaea17f9016c0a013bd` |
| 401 | 客套话与礼貌用语较长说法补漏 | merge | 10 | 0 | 10 | 0 | 90.00% | 100.00% | `aeeddc1b5bd030412f7b6f4ec874cc61abe6dcd3` |
| 402 | 道歉与感谢较长表达补漏 | merge | 10 | 0 | 10 | 0 | 100.00% | 100.00% | `3c55e9207d1402a0664440bb28151360488ef7d8` |
| 403 | 祝福与问候较长表达补漏 | merge | 10 | 0 | 10 | 0 | 80.00% | 100.00% | `e3b94e63c6c6ccfb7cac044d60288941146fdc1b` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round30_exclusions.tsv`，共 94 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality: awkward collocation, missing function word or decorative slogan`：15 条。
- `duplicate:local_core_zh_pinyin.tsv`：57 条。
- `quality: formulaic variation, awkward courtesy or uncertain established usage`：22 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

392 个含多音字的词在各批复核表逐字记录；3 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

还请、还望和大家都还好吧中的还取 hai；多有叨扰的叨取 dao，不取默认 tao；衷心地感谢、由衷地感谢中的地为助词 de，不取默认 di。劳驾取 lao jia，承蒙取 cheng meng，长寿取 chang shou，见谅取 jian liang，得偿所愿取 de chang suo yuan。轻声按本字写，逐条审阅完整全拼并为含多音字的词逐字记录。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 260/260 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，236017 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round30_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.97% → 85.97% | +0.00 | 98.11% → 98.11% |
| 逐词输入·无前文 | 20626 | 77.98% → 77.98% | +0.00 | 97.19% → 97.19% |
| 留出整句 | 4126 | 58.07% → 58.07% | +0.00 | 73.44% → 73.44% |
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
- `docs/lexicon-keep-adding-round30.md`
- `resources/dictionary/expansion/batch398_authored_draft.txt`
- `resources/dictionary/expansion/batch398_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch399_authored_draft.txt`
- `resources/dictionary/expansion/batch399_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch400_authored_draft.txt`
- `resources/dictionary/expansion/batch400_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch401_draft.txt`
- `resources/dictionary/expansion/batch401_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch402_draft.txt`
- `resources/dictionary/expansion/batch402_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch403_draft.txt`
- `resources/dictionary/expansion/batch403_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_398.md`
- `resources/dictionary/expansion/batch_399.md`
- `resources/dictionary/expansion/batch_400.md`
- `resources/dictionary/expansion/batch_401.md`
- `resources/dictionary/expansion/batch_402.md`
- `resources/dictionary/expansion/batch_403.md`
- `resources/dictionary/expansion/lexicon_batch_398.applied.json`
- `resources/dictionary/expansion/lexicon_batch_398.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_398.json`
- `resources/dictionary/expansion/lexicon_batch_398.tsv`
- `resources/dictionary/expansion/lexicon_batch_398.validation.json`
- `resources/dictionary/expansion/lexicon_batch_399.applied.json`
- `resources/dictionary/expansion/lexicon_batch_399.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_399.json`
- `resources/dictionary/expansion/lexicon_batch_399.tsv`
- `resources/dictionary/expansion/lexicon_batch_399.validation.json`
- `resources/dictionary/expansion/lexicon_batch_400.applied.json`
- `resources/dictionary/expansion/lexicon_batch_400.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_400.json`
- `resources/dictionary/expansion/lexicon_batch_400.tsv`
- `resources/dictionary/expansion/lexicon_batch_400.validation.json`
- `resources/dictionary/expansion/lexicon_batch_401.json`
- `resources/dictionary/expansion/lexicon_batch_401.tsv`
- `resources/dictionary/expansion/lexicon_batch_402.json`
- `resources/dictionary/expansion/lexicon_batch_402.tsv`
- `resources/dictionary/expansion/lexicon_batch_403.json`
- `resources/dictionary/expansion/lexicon_batch_403.tsv`
- `resources/dictionary/expansion/probes/batch398_probe.tsv`
- `resources/dictionary/expansion/probes/batch399_probe.tsv`
- `resources/dictionary/expansion/probes/batch400_probe.tsv`
- `resources/dictionary/expansion/probes/batch401_probe.tsv`
- `resources/dictionary/expansion/probes/batch402_probe.tsv`
- `resources/dictionary/expansion/probes/batch403_probe.tsv`
- `resources/dictionary/expansion/round30_exclusions.tsv`
- `resources/dictionary/expansion/round30_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
