# 本地拼音 0.8.4

Windows 10/11 x64 离线中文拼音输入法。输入核心是原生 C++20 / Windows TSF，设置界面使用 Electron、React、TypeScript 和 Vite。关闭设置后设置程序退出，输入核心继续独立工作。安装包自带运行时，使用时无需联网，也不需要另外安装 Node.js 或 Python。

![候选窗口](docs/images/candidate-single-line.png)

## 0.8.4 词库扩充

- 新增 1,642 条词（批次 135–147）：补齐《通用规范汉字表》一级全部缺字与二级 650 个缺字（单字按标准读音可打），常用的机构、地名、历史与思想用语、普通词语、成语，以及时事新词、港台用语和科技新词。最常用 1 万词的收录率 91.3% → 95.6%。
- 五个测试集首选率均不低于 0.8.3（逐词输入 85.95% → 85.98%，留出整句 57.22% → 57.27%）。详见 [LEXICON_COVERAGE](docs/LEXICON_COVERAGE.md)。
- 引擎按“字”而不是 UTF-16 单元判断多字词，扩展 B 区汉字不再被当作两字词加分。扩展区汉字暂不收入词库，待前文模型支持后加入。

## 0.8.3 修正

- 安装程序在安装完成后停在 100% 不动（安装本身已成功）：安装进程结束的状态只在一次刷新里有效，下一次就丢失。现已修正。
- 等待管理员权限确认期间，进度停在 0 并提示在弹出窗口中选择“是”，不再假装在复制文件；拒绝或超时后说明原因，可重试。
- Microsoft Store 应用中的候选窗口已在 Notepads 中实机确认显示正常。

## 0.8.2 更新

- **候选框设置按钮。** 候选窗口右侧新增齿轮按钮，点击打开设置（Microsoft Store 应用中不显示，那里无法启动外部程序）。
- **选词记忆修正。** 对同一串拼音，最近选过的词排到最前，选得越近越靠前、次数多的其次。此前输入 annui 时“安徽”永远第一：annui 不是标准拼音，候选来自拼写容错，学习加分追不上容错扣分；现在学习记录直接决定顺序，不再与打分相加。保存学习记录和个人词库时，若文件正被杀毒软件或索引服务短暂占用，会稍等后重试，不再偶尔丢失一次保存。
- **新的安装程序。** 单窗口、无系统边框，图标、标题、一个“立即安装”按钮，进度条和完成页，跟随浅色/深色主题与显示缩放。复制、注册与启用仍由经过验证的 Inno Setup 包在后台静默完成（会弹出一次管理员权限确认）。
- **词库覆盖率评估。** 见 [LEXICON_COVERAGE](docs/LEXICON_COVERAGE.md)：一级常用字 99.6%，最常用 5,000 词 95.9%，日常句子约九成的词直接在词库中。

![安装程序](docs/images/installer.png)

## 0.8.1 修正

- CapsLock 开启或按住 Shift 时，字母直接输出大写；正在输入拼音时，大写字母按原样加入输入串，候选为原样文字，空格上屏。
- Microsoft Store（UWP）应用中，候选窗口以输入框所在窗口为所有者创建，避免被系统隐藏。

## 0.8.0 更新

**更准的前文排序。** 新的字符 4 元语言模型（插值修正 Kneser-Ney，约 1,030 万字统计、15.6 MB、内存映射共享）取代了原来约 20 万字的计数打分。输入新拼音时，候选按光标前已经输入的文字排序：“他跑”之后输入 de 首选“得”，“没有”之后输入 shijian 首选“时间”。在与训练数据分开的测试集上：

| 测试集 | 0.7.9 首选 | 0.8.0 首选 |
| --- | ---: | ---: |
| 逐词输入（有前文，10,466 例） | 71.3% | 85.7% |
| 留出整句（2,082 例） | 30.1% | 56.0% |
| 项目原有四组整句用例（299 例） | 56.5% | 66.6% |

前文对首选的贡献从 1.1 提高到 12.2 个百分点。不能读取光标前文字的应用，改用本输入法刚在同一输入框上屏的文字作为参考（仅在内存中，短时有效）。详见 [前文语言模型](docs/CONTEXT_MODEL.md)。

**联想改为可选。** 上屏后自动推荐下一个词的“联想”默认关闭，可在设置的“输入”页打开；0.7 保存的旧设置不会自动打开它。见 [前文排序与联想](docs/CONTEXT_PREDICTION.md)。

**新的候选窗口。** 上方显示带音节分隔的拼音（如 `ni'hao'shi'jie`），下方单行候选，选中项为蓝色；圆角抗锯齿并带柔和阴影（逐像素透明窗口），跟随浅色/深色主题。候选多于一页时显示左右翻页按钮。

**图标更清晰。** 保持原有“对话气泡 + P| + 键盘”设计，重新绘制为矢量并逐尺寸渲染；32 px 及以下省略键盘、放大气泡，P 对齐整像素。任务栏的“中/英”指示改为按系统真实 DPI 运行时绘制，跟随任务栏浅色/深色与高对比度主题。

![图标尺寸](docs/images/icon-sizes.png)

**重做的设置界面。** Windows 11 风格分组设置、品牌蓝配色、与真实候选窗口一致的实时预览，以及“参考前文 / 不参考前文”对比试用。

![设置界面](docs/images/interface.png)

## 安装与使用

运行安装包后按 Win＋空格选择“本地拼音”。升级后重新打开正在输入的应用。

- CapsLock 开启或按住 Shift 时输入大写英文字母。
- 空格确认当前候选，数字 1–9 选择当前页对应候选，左右箭头翻页，Esc 取消，Enter 上屏原始拼音。
- Ctrl＋Space 切换中英文；可在设置中打开“按 Shift 切换中英文”。
- 支持全拼、连续拼音与整句候选、简拼、补全、单次拼写容错、中文标点、鼠标选词、个人词库和本地学习。长句和同音词仍可能需要选词。

## 构建

需要 Visual Studio C++ 工具和 Windows SDK、CMake/Ninja、Python 3、Node.js 22.12 或更高版本及 Inno Setup 6。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build.ps1
```

脚本构建并测试原生组件、安装前端依赖、编译 Vite 与 Electron、打包桥接与运行数据，在 dist 中生成安装包及校验文件：先用 Inno Setup 生成负责复制与注册的内层包，再把它嵌入安装界面 `LocalPinyinInstaller`（`src/installer`）。`LocalPinyinInstaller.exe --preview` 模拟一次安装而不改动系统，`--capture <目录>` 把各状态渲染为 PNG。首次需要联网下载依赖和 Electron。desktop 中运行 `npm ci` 与 `npm run dev` 可开发设置界面（需先构建原生桥接）。

语言模型与图标的生成脚本：`tools/fetch_context_corpus.py`、`tools/build_context_lm.py`、`tools/generate_brand_icon.py`、`tools/generate_mode_icons.py`；生成时使用的 Python 包不进入安装包。补词时用 `tools/export_lexicon_gaps.py` 导出缺字缺词清单，用 `tools/accuracy_snapshot.py` 对比补词前后五个测试集的准确率。补词任务的完整规则与流程见 [LEXICON_EXPANSION_TASK](docs/LEXICON_EXPANSION_TASK.md)。

## 数据与验证

个人设置、词库和学习保存在 `%LOCALAPPDATA%\LocalPinyinIME`；Electron 缓存在其中的 desktop 子目录。学习最多保存 2,048 条记录；清除学习保留个人词库，卸载保留个人数据。前文查询和试用都在本机完成。

36 项原生测试（含跨进程桥接、真实 TSF 编辑会话、候选布局与新增的前文模型测试）和 15 个 Electron 桌面流程通过。未做干净机器上的完整安装卸载验收。兼容范围见 [COMPATIBILITY](docs/COMPATIBILITY.md)。

## 许可

项目代码、原创词库与联想关系采用 MIT；标准词库为 Apache-2.0。前文语言模型只包含统计数据，来源包括 Tatoeba（CC BY 2.0 FR）、中文维基新闻（CC BY 2.5）、Mozilla Common Voice（CC0）与项目原创及公有领域语料，署名见 `resources/THIRD_PARTY_NOTICES.txt`。上游页面标题整理的名称索引有独立来源记录，不统一声称 MIT 授权，参见 [历史来源审计](docs/remote-lexicon-audit-074.md) 及 [此前上游审计](docs/remote-lexicon-audit-076.md)。Electron/React 采用 MIT。
