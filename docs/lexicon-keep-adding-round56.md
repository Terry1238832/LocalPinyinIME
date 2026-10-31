# 词库续加第56轮（批次 556–561）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `1ade1807369a14839465c42e38def62367746f76`，引擎版本 1.8.2。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：261305 → 262475，新增 **1170 条**。
- 权重 6000 / 5500 / 5000：0 / 1170 / 0 条。
- 最终 SHA-256：`46BC63DF48EA091CAEBA1DF70E55445A308A7D11957D4A4FC7A8C5BFE4AC2991`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：聊天口语。补充节日问候与祝福回应、好消息分享与祝贺、低落受挫时的安慰与陪伴三类常见说法。逐条自拟，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 556 | 节日问候与祝福回应聊天 | curate | 364 | 0 | 364 | 0 | 88.46% | 100.00% | `ef4b7b98eee558d27eb9e6a98d06edcc147944b1` |
| 557 | 好消息分享与祝贺回应聊天 | curate | 416 | 0 | 416 | 0 | 87.26% | 100.00% | `307958d0c41ca90ab3356f485b53b9b01cd43079` |
| 558 | 低落受挫时的安慰与陪伴聊天 | curate | 360 | 0 | 360 | 0 | 81.67% | 100.00% | `ccf3e0604b0575673eae5f742b519e6d6a657af2` |
| 559 | 节日问候与祝福回应较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 90.00% | `9242f3a729991f49e900ee2e2fa089c8e287ca86` |
| 560 | 好消息分享与祝贺回应较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 100.00% | `b5dc98314f2192235697af3ab280256f523b7bce` |
| 561 | 低落受挫时安慰与陪伴较长说法 | merge | 10 | 0 | 10 | 0 | 80.00% | 90.00% | `7562cf5341897eb56b5543c022d5c2891286d770` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round56_exclusions.tsv`，共 187 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:standard_zh_pinyin.tsv`：8 条。
- `duplicate:local_core_zh_pinyin.tsv`：141 条。
- `quality: clipped greeting; full greeting is more natural`：1 条。
- `duplicate:batch556`：5 条。
- `outside_batch_length_scope`：25 条。
- `duplicate:batch557`：1 条。
- `quality: use the clearer common question wording`：1 条。
- `quality: incomplete wording`：2 条。
- `quality: missing particle makes the offer clipped`：1 条。
- `quality: contextual 了 read is ambiguous here; replaced with a clearer complete sentence`：1 条。
- `quality: 都 in the polyphonic phrase is misread as du; replaced with a clearer sentence`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1027 个含多音字的词在各批复核表逐字记录；没有词键偏离 pypinyin 整词默认。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

关键语境读音：称心如意的“称”读 chen；暖和的“和”读 huo；不着急的“着”读 zhao；睡一觉的“觉”读 jiao；擅长的“长”读 chang；调整的“调”读 tiao；“谁都”按口语读 shei；重新的“重”读 chong。长句中“都会”的“都”会被误读为 du，已排除原说法并改用“会一直”；“有了结果”中的“了”也通过换句消除歧义。其余多音字逐字记录在复核表中。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 188/188 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，262475 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round56_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round56.md`
- `resources/dictionary/expansion/batch556_authored_draft.txt`
- `resources/dictionary/expansion/batch556_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch557_authored_draft.txt`
- `resources/dictionary/expansion/batch557_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch558_authored_draft.txt`
- `resources/dictionary/expansion/batch558_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch559_draft.txt`
- `resources/dictionary/expansion/batch559_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch560_draft.txt`
- `resources/dictionary/expansion/batch560_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch561_draft.txt`
- `resources/dictionary/expansion/batch561_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_556.md`
- `resources/dictionary/expansion/batch_557.md`
- `resources/dictionary/expansion/batch_558.md`
- `resources/dictionary/expansion/batch_559.md`
- `resources/dictionary/expansion/batch_560.md`
- `resources/dictionary/expansion/batch_561.md`
- `resources/dictionary/expansion/lexicon_batch_556.applied.json`
- `resources/dictionary/expansion/lexicon_batch_556.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_556.json`
- `resources/dictionary/expansion/lexicon_batch_556.tsv`
- `resources/dictionary/expansion/lexicon_batch_556.validation.json`
- `resources/dictionary/expansion/lexicon_batch_557.applied.json`
- `resources/dictionary/expansion/lexicon_batch_557.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_557.json`
- `resources/dictionary/expansion/lexicon_batch_557.tsv`
- `resources/dictionary/expansion/lexicon_batch_557.validation.json`
- `resources/dictionary/expansion/lexicon_batch_558.applied.json`
- `resources/dictionary/expansion/lexicon_batch_558.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_558.json`
- `resources/dictionary/expansion/lexicon_batch_558.tsv`
- `resources/dictionary/expansion/lexicon_batch_558.validation.json`
- `resources/dictionary/expansion/lexicon_batch_559.json`
- `resources/dictionary/expansion/lexicon_batch_559.tsv`
- `resources/dictionary/expansion/lexicon_batch_560.json`
- `resources/dictionary/expansion/lexicon_batch_560.tsv`
- `resources/dictionary/expansion/lexicon_batch_561.json`
- `resources/dictionary/expansion/lexicon_batch_561.tsv`
- `resources/dictionary/expansion/probes/batch556_probe.tsv`
- `resources/dictionary/expansion/probes/batch557_probe.tsv`
- `resources/dictionary/expansion/probes/batch558_probe.tsv`
- `resources/dictionary/expansion/probes/batch559_probe.tsv`
- `resources/dictionary/expansion/probes/batch560_probe.tsv`
- `resources/dictionary/expansion/probes/batch561_probe.tsv`
- `resources/dictionary/expansion/round56_exclusions.tsv`
- `resources/dictionary/expansion/round56_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
