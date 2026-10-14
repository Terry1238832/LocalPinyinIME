# 0.7.5 vocabulary authoring audit

Status: **passed**. Immutable 0.7.4 baseline: `d62c22c94d25b40e7da7c966f7fcfd78e4530d68`.

Baseline local rows: 95,324; current: 101,386. All 267 complete typing answers are excluded from new batches.

The audit independently reconstructs each batch from frozen drafts, preset category weights, and contextual pronunciation decisions. It checks every polyphone review membership, display/key deduplication across all three runtime layers, authoring-code/draft/review SHA, and exact canonical merge bytes. Existing rows and weights are unchanged.

Prior source records and reviewed source policy are preserved exactly. Batches 012–015 retain reference-derived provenance and its recorded limitations; this audit does not relabel the entire local dictionary as first-party MIT material.

| Batch | Added entries | Polyphone reviews | Semantic default corrections | Reading variant choices |
| --- | ---: | ---: | ---: | ---: |
| lexicon_batch_018 | 3877 | 3015 | 90 | 6 |
| lexicon_batch_019 | 2185 | 1692 | 53 | 11 |

Per-character compatibility and recorded review membership are structural evidence. Independent manual naturalness/pronunciation QA: **passed**. Native coverage and public sentence replay are reported separately by the release coordinator.

Machine-readable evidence and all bound SHA values: [lexicon-075-authoring-audit.json](lexicon-075-authoring-audit.json).
