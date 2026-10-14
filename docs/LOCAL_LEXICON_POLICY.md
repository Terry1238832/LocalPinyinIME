# LocalPinyinIME Local Lexicon Policy

LocalPinyinIME uses a small layered dictionary model:

1. `resources/dictionary/core_zh_pinyin.tsv`
2. `resources/dictionary/local_core_zh_pinyin.tsv`
3. `resources/dictionary/standard_zh_pinyin.tsv` (Apache-2.0 Rime/AOSP short vocabulary and decoding reference)
4. `%LOCALAPPDATA%\LocalPinyinIME\user_lexicon.tsv`

`local_core_zh_pinyin.tsv` is reserved for project-maintained entries that are small, explicit, and publicly reviewable. It is not a place to bulk-import third-party dictionaries.

The current `local_core_zh_pinyin.tsv` is the project-maintained built-in common lexicon v2. It expands the earlier v1 foundation with first-party daily sentence components and short natural phrases for time, weather, requests, questions, life, study, work, technology, shopping, food, transport, spoken patterns, and connectors.

Frequency values in this file are coarse relative ranking weights maintained by the project, such as `9000`, `7000`, `5000`, `3000`, and `1000`. They are not external corpus statistics and must not be described as measured real-world language frequency.

The v2 lexicon is reviewed for file format, duplicate `pinyin + word` keys, required common entries, required category coverage, sentence-input benchmarks, and obvious scope violations. It has not received exhaustive formal linguistic review for every entry.

The current user lexicon is private to the Windows user account. It lives at `%LOCALAPPDATA%\LocalPinyinIME\user_lexicon.tsv`, must not be committed to Git, must not be packaged into releases, and must not be uploaded by project tools.

The project-maintained local layer contains individually authored vocabulary and, from the user-requested upstream update in 0.7.4, separately attributed reference-derived word/name/title indexes. Those indexes do not inherit a blanket MIT source-content license; exact source-page manifests are incomplete and technical checks are not exhaustive linguistic or rights review. See `docs/remote-lexicon-audit-074.md`, the source registry and bundled notices. Version 0.5 adds a separate, pinned Apache-2.0 Rime/AOSP standard layer; it does not import rime-ice or commercial IME dictionaries. User entries override references. The standard source, transformations, checksum, and full license are recorded in `standard_zh_pinyin.metadata.json` and `resources/THIRD_PARTY_NOTICES.txt`.

Version 0.7.6 also records the user's requested 020–023 repository contributions as upstream vocabulary/name indexes, with immutable contribution commits and incomplete external page manifests; see `docs/remote-lexicon-audit-076.md`. Unknown permissions remain unknown. Original project-generated pronunciation keys and ranking edits are distinguished from referenced names, trademarks and protected source-page material. First-party 024+ phrases have a single AI author contextual review plus read-only structural reconstruction; neither a second language editor nor an exhaustive official-name phonetic review is claimed.

`context_zh.tsv` contains only short character statistics from attributed public-domain prose and original daily text. Its training works exclude the three article validation works. No frozen validation sentence is inserted as a new full-sentence dictionary candidate. Training sources and weights are recorded in `authoring/sources.json` and `context_zh.metadata.json`.

Before any future import, the project must record:

- source name;
- authorization or license;
- import date;
- distribution scope;
- transformation process;
- reviewer.

No document in this repository should claim that every possible dictionary source is free of copyright restrictions. Each source must be reviewed on its own terms before it is added.

Machine-readable source review records live in `resources/dictionary/sources/lexicon_sources.json`.
