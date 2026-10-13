# 第一方沟通与日常事务词汇批次 010

本批逐项手写并精选职场沟通、项目协作、客服售后、快递账单、预约办事、运动娱乐、家庭照料、作息饮食、情绪交流、日程安排、消费收支和礼貌协商的常用词与自然短表达。作者按实际语用选择词条，压缩相近动作和通知状态，删除不够自然的片段；没有使用人称、动作、对象的笛卡尔模板，没有导入外部词库、文章、用户输入或评测答案。

冻结作者候选 2,635 项，排除内部重复 3 项、各层已有显示词 15 项和自然性复核不通过的 4 项后，新增 2,613 项。完整作者草稿、逐词审音表和 SHA-256 保留在本目录。自然性排除项是“恍然想起来”“留意身体啊”“先抓紧要紧的”“擦边球了”，保留原作者候选以便审计，没有为填数量补回。

| 场景 | 新词数 |
| --- | ---: |
| 预约与办事 | 185 |
| 客服售后 | 197 |
| 快递与账单 | 287 |
| 情绪交流 | 237 |
| 娱乐休闲 | 270 |
| 日常澄清 | 67 |
| 日常协商 | 69 |
| 生活观察与动作补语 | 66 |
| 作息饮食与日常健康 | 179 |
| 家庭照料 | 224 |
| 个人消费收支 | 67 |
| 个人日程 | 68 |
| 项目协作 | 317 |
| 礼貌与相处边界 | 60 |
| 运动训练 | 192 |
| 职场沟通 | 128 |

实际输出为 3 字 3 项、4 字 1,736 项、5 字 738 项、6 字 136 项，全部符合 2–6 汉字约束。权重仅采用固定粗分的 5,000、5,500、6,000；它们是项目相对排序偏好，不是语料、网络、搜索或商业输入法频率，没有根据后续原生测试调权，也没有要求每个新词成为首选。

作者阶段使用 pypinyin 0.55.0 检查默认拼音，不进入运行组件。本批对任何含有多个无声调可能读音字符的词均要求显式复核，而非沿用旧批的固定多音字字符清单。2,061 个词的含义和读音逐项记录在 `batch010_pronunciation_review.tsv`；其 SHA-256 为 `639c6e69ef80e0d06e6be976b6dc6e94f12eb426e1082d9fe6a58827e0e6b9f0`。逐字可能读音、纯小写 ASCII 拼音、长度、非负固定权重、显示词唯一和键词对唯一检查均通过。这是本批语境复核记录，并非正式语言学审定声明。

共纠正 26 项自动默认读音。例如“发票没收到”“球没过网”的“没”按否定读 mei；“重开发票”“过期需重办”读 chong；“还款凭证”“还款未到账”读 huan；“差旅报销单”读 chai；“绳子太长”“调整绳长”“指甲太长”读 chang；音量、伴奏、灯光和屏幕的“调”读 tiao；“学系鞋带”读 ji；睡眠语境的“觉”读 jiao；“尊重选择”读 zhong；表示“必须”的“得”读 dei；“一点薄礼”的“薄礼”读 boli；表示考虑的“让我再琢磨”用 zuomo。其余复核项明确确认语境默认读音。已有显示词不增加自动读音别名，旧词的拼音和权重未改。

以下主源页面仅读单个字词的读音或该单词在标准中的一行，未导入词条列表、定义正文、例句或标准词表：[薄礼 bó lǐ](https://dict.revised.moe.edu.tw/dictView.jsp?ID=13333&la=0&powerMode=0)、[得空 dé kòng](https://dict.revised.moe.edu.tw/dictView.jsp?ID=42006&la=1&powerMode=0)、[得表示必须读 děi](https://dict.mini.moe.edu.tw/SearchIndex/word_detail?breadcrumbs=Search_%E5%BE%97_one&dictSearchField=%E5%BE%97&wordID=D0002532)、[教育部标准中琢磨一词的 zuómo 读音](https://www.moe.gov.cn/jyb_sjzl/ziliao/A19/201010/W020220124393643101738.pdf)、[教育部审音表中粘贴的 zhān 读音](https://www.moe.gov.cn/jyb_sjzl/ziliao/A19/201001/W020190416497956176438.pdf)、[差旅费 chāi lǚ fèi](https://dict.concised.moe.edu.tw/dictView.jsp?ID=31706)、[还钱 huán qián](https://dict.concised.moe.edu.tw/dictView.jsp?ID=20767&la=0&powerMode=0)。

正式冻结前对 core、local_core、standard、001–009 的所有键词对及显示词去重，并排除三套 `typing_*.tsv` 共 267 案例的全部完整答案。应用前再次核对各层及全部夹具 SHA；本批与这些完整答案碰撞为零，没有从评测答案编词。已用 `tools/merge_lexicon_batch.py` 正式合并，并恢复 canonical metadata 中准确的“local_core 第一方作者层、standard 独立授权层”来源说明。本批登记为项目 MIT 第一方来源，见 `sources/lexicon_sources.json`。

冻结 TSV SHA-256 为 `77b827c0b7440e34ce76f3ad38275508f06e37cb6aa6791ab739e624c1db1cec`；canonical 合并前为 `253019237faca0af4cc7bc17453bc0a8cfc7d0a6391b83b8162d0a2da8b1c75d`，合并后为 `c18a4d7e752a6357eddf322dbd121fb00498a9dfc168cdede9d5f8f85a0a7b14`。local_core 从 40,943 增至 43,556 项；三层原始行共 99,674，按键词对合并后的有效词条为 90,457，较之前 87,844 增加 2,613。所有作者、审音、脚本、TSV 与 JSON 采用 UTF-8 无 BOM 和 LF。作者冻结脚本与应用脚本会拒绝重写已应用批次。

`validate_local_core_lexicon.py --min-entries 8000 --max-entries 60000` 已通过，确认 canonical 43,556 条、metadata 数量与 SHA 一致。

本轮构建刷新 `.build/bin` 的五份运行 TSV 后，已执行 `validate_lexicon_batch_010_native.py`。脚本先确认运行资源与源码五份 TSV 哈希全部一致，再直接查询冻结 TSV 的全部 2,613 个键，隔离用户词库与学习数据，不学习预期文本。实际 top1 为 2,256/2,613（86.3375%），top9 为 2,613/2,613（100%），前九缺失 0 项，最多返回 9 个候选；357 项非首选词和全部完整排名保留在 `lexicon_batch_010.validation.json`。

实际查询耗时 p50 4.403 ms、p95 8.677 ms、最大 16.259 ms，仅包含引擎查询，不包含系统事件与编辑会话耗时。Eval SHA-256 为 `01d2f7c4e091833ee906ab361a734b64652ad6b7544d043c70afee81e1bb92f9`；原生验证报告 SHA-256 为 `4d301e5e2159a3bbec15223eb8c2510a4057d4d08ecf1be8d01305b84d677640`。冻结 TSV、Eval 和源码/运行五份 TSV 执行前后哈希完全一致，资源哈希保留在 validation/applied。原生覆盖结果为 `passed`；未依据本次结果修改任何读音、词面、权重或排序代码，也未把 top9 覆盖当作全部首选。
