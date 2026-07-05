# AI 辅助词库补丁提示词

你要为 LocalPinyinIME 生成一个小型第一方候选词补丁 TSV。请只输出 TSV 内容，不要解释。

硬性要求：

- UTF-8。
- 每行格式：`pinyin<TAB>word<TAB>frequency`。
- 拼音只能包含 `a-z`，小写、无声调、无空格、无数字、无符号。
- 词语不能为空，不能包含制表符或换行。
- 频率使用相对排序权重：`9000 / 7000 / 5000 / 3000 / 1000`。
- 不输出重复的 `pinyin + word`。
- 不输出生僻、无法确认读音、多音不确定、品牌、人名、影视角色、长句或网络流行语。
- 不使用 rime-ice、搜狗、百度、微软、Google、网页抓取或第三方词库来源。
- 不包含用户隐私、聊天内容、文件路径、账号、邮箱、电话号码。

推荐先生成 20 到 80 行，便于人工审阅。补丁必须经过：

```powershell
python .\tools\merge_local_core_lexicon_patch.py --patch .\patch.tsv
```

dry-run 通过后，才可由维护者决定是否应用。
