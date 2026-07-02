# LocalPinyinIME Local Lexicon Policy

LocalPinyinIME uses a small layered dictionary model:

1. `resources/dictionary/core_zh_pinyin.tsv`
2. `resources/dictionary/local_core_zh_pinyin.tsv`
3. `%LOCALAPPDATA%\LocalPinyinIME\user_lexicon.tsv`

`local_core_zh_pinyin.tsv` is reserved for project-maintained entries that are small, explicit, and publicly reviewable. It is not a place to bulk-import third-party dictionaries.

The current `local_core_zh_pinyin.tsv` is the project-maintained built-in common lexicon v2. It expands the earlier v1 foundation with first-party daily sentence components and short natural phrases for time, weather, requests, questions, life, study, work, technology, shopping, food, transport, spoken patterns, and connectors.

Frequency values in this file are coarse relative ranking weights maintained by the project, such as `9000`, `7000`, `5000`, `3000`, and `1000`. They are not external corpus statistics and must not be described as measured real-world language frequency.

The v2 lexicon is reviewed for file format, duplicate `pinyin + word` keys, required common entries, required category coverage, sentence-input benchmarks, and obvious scope violations. It has not received exhaustive formal linguistic review for every entry.

The current user lexicon is private to the Windows user account. It lives at `%LOCALAPPDATA%\LocalPinyinIME\user_lexicon.tsv`, must not be committed to Git, must not be packaged into releases, and must not be uploaded by project tools.

This project does not import rime-ice, Sogou, Baidu, Microsoft, Google, scraped web corpus data, or any other third-party bulk lexicon in this foundation stage.

Before any future import, the project must record:

- source name;
- authorization or license;
- import date;
- distribution scope;
- transformation process;
- reviewer.

No document in this repository should claim that every possible dictionary source is free of copyright restrictions. Each source must be reviewed on its own terms before it is added.

Machine-readable source review records live in `resources/dictionary/sources/lexicon_sources.json`.
The offline import preparation tool is `tools/import_lexicon_source.py`; it only accepts explicit local files and refuses sources that are not both `review_status = approved` and `included_in_release = true`.
