# LocalPinyinIME Lexicon Sources Policy

LocalPinyinIME must keep dictionary sources auditable before any entry is included in a release.

The independently authored vocabulary is tracked as `first_party_curated`, `first_party_curated_v2` and individually reviewed author batches in `resources/dictionary/sources/lexicon_sources.json`. Version 0.5 also includes an independently reviewed Apache-2.0 Rime/AOSP standard layer with a pinned upstream commit, normalization recipe, checksums, and bundled license. The standard layer supplies missing short vocabulary and sentence-decoding reference frequencies; it preserves project/user direct-candidate overrides. It does not use rime-ice or proprietary commercial input-method data.

Version 0.7.4 also integrates the user's requested upstream batches 012–015 as separately recorded reference-derived word/name/title indexes. They are not labeled first-party authored or blanket MIT content. Known sites, immutable contribution commits, site-content terms and missing exact page/revision manifests are recorded in the source registry, [upstream audit](remote-lexicon-audit-074.md) and bundled notices. Technical inclusion of these vocabulary identifiers is not a grant to copy protected source pages, artwork, logos or compiled source-site lists. Unknown source-site permissions remain unknown rather than `commercial_use_allowed = true`. Original pronunciation keys and ranking weights are distinguished from referenced names and page content. All existing and future imports of licensed source material remain subject to the review requirements below.

Future sources must be reviewed before import. A source may enter a release dictionary only when:

- `review_status` is `approved`;
- `included_in_release` is `true`;
- license and redistribution terms are explicit;
- attribution, share-alike, commercial-use, and derivative-work requirements are recorded;
- the transformation and validation process is reproducible.

Do not treat "published on GitHub" or a vague README sentence as permission to copy or redistribute a lexicon. Do not claim a source has no copyright restrictions unless the source explicitly says so.

Unreviewed sources must remain `review_status = pending` and `included_in_release = false`. Pending sources may be used only for local review metadata, not for release dictionaries.

## First-party v2 Layers

The first-party v2 expansion is reviewed in small layers:

| Layer | Target size | Entry type | Frequency policy | Default built-in | Optional install |
| --- | ---: | --- | --- | --- | --- |
| `common_single_chars` | 300-600 | high-value single characters that unlock segmentation | 7000-9000 only for very common characters | yes | no |
| `common_words_2char` | 3000-6000 | common two-character daily words | 5000-9000 by broad tier | yes | no |
| `common_words_3char` | 1000-2500 | stable three-character expressions and terms | 3000-7000 | yes | no |
| `common_words_4plus` | 1000-3000 | common four-plus-character phrases, not long sentences | 1000-5000 | yes, after review | maybe |
| `daily_phrases` | 1000-3000 | greetings, requests, everyday sentence fragments | 3000-7000 | yes | maybe |
| `spoken_patterns` | 500-1500 | reusable spoken fragments such as "我想", "请帮我" | 3000-7000 | yes | maybe |
| `education_tech_terms` | 500-1500 | school, computer, network, file, software terms | 1000-5000 | yes | maybe |
| `proper_nouns_opt_in` | reviewed batches only | place names, schools, organizations, product names | 1000-3000 | no by default | yes |

The most valuable categories for full-sentence input are common short function phrases, daily verbs, complements, time/weather fragments, and everyday object words. Low-frequency proper nouns should not displace common words in default ranking.

Every expansion batch must pass offline checks: pinyin contains only `a-z`, words contain no tabs or newlines, frequency is non-negative, duplicate `pinyin + word` keys are rejected, length/category statistics are reported, and high-frequency smoke phrases are checked.
