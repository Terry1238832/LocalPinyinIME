"""Convert a pinned Apache-2.0 Rime/AOSP lexicon to compact runtime TSV.

Short vocabulary and sentence-decoding reference frequencies are generated.
Project/user direct-word ranking overrides are kept by the runtime loader.
Upstream copyright/license and modifications are recorded alongside the data.
The network is used for authoring only. The input method remains offline.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import re
from urllib.request import Request, urlopen

REPOSITORY = "rime/rime-pinyin-simp"


def download(url):
    with urlopen(Request(url, headers={"User-Agent": "LocalPinyinIME-dictionary-authoring"}), timeout=45) as response:
        return response.read()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--dictionary-dir", type=Path, required=True)
    p.add_argument("--cache-dir", type=Path, required=True)
    p.add_argument("--holdout", type=Path, required=True)
    p.add_argument("--commit", default="0c6861ef7420ee780270ca6d993d18d4101049d0")
    a = p.parse_args()
    commit = a.commit or json.loads(download(f"https://api.github.com/repos/{REPOSITORY}/commits/master"))["sha"]
    if not re.fullmatch("[0-9a-f]{40}", commit): raise ValueError("Invalid pinned commit")
    root = f"https://raw.githubusercontent.com/{REPOSITORY}/{commit}/"
    raw = download(root+"pinyin_simp.dict.yaml")
    license_text = download(root+"LICENSE")
    authors_text = download(root+"AUTHORS")
    if not license_text.lstrip().startswith(b"Apache License"): raise ValueError("Unexpected upstream license")
    a.cache_dir.mkdir(parents=True, exist_ok=True)
    (a.cache_dir / "pinyin_simp.dict.yaml").write_bytes(raw)
    (a.cache_dir / "LICENSE").write_bytes(license_text)
    (a.cache_dir / "AUTHORS").write_bytes(authors_text)
    known = set()
    for filename in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv"):
        for line in (a.dictionary_dir / filename).read_text(encoding="utf-8").splitlines():
            if line and not line.startswith("#"):
                key, word, _ = line.split("\t")
                known.add((key, word))
    excluded = {line.split("\t")[1] for line in a.holdout.read_text(encoding="utf-8").splitlines()
                if line and not line.startswith("#") and len(line.split("\t")) >= 2 and len(line.split("\t")[1]) >= 6}
    entries = {}
    all_frequencies = {}
    skipped = Counter()
    in_data = False
    for line in raw.decode("utf-8").splitlines():
        if line == "...": in_data = True; continue
        if not in_data or not line or line.startswith("#"): continue
        fields = line.split("\t")
        if len(fields) != 3: skipped["format"] += 1; continue
        word, syllables, frequency_text = fields
        key = syllables.replace(" ", "").replace("ü", "v").replace("u:", "v")
        if not re.fullmatch(r"[\u3400-\u9fff]{1,8}", word) or not re.fullmatch("[a-z]+", key):
            skipped["characters"] += 1; continue
        try: frequency = int(frequency_text)
        except ValueError: skipped["frequency"] += 1; continue
        if frequency < (10 if len(word) == 1 else 1): skipped["rare"] += 1; continue
        if word in excluded: skipped["holdout"] += 1; continue
        all_frequencies[(key, word)] = frequency
        if (key, word) in known: skipped["existing_reference_only"] += 1
    maxima = {group: max(math.log1p(frequency) for (_, word), frequency in all_frequencies.items()
                        if (len(word) == 1) == (group == "single")) for group in ("single", "multi")}
    for (key, word), frequency in all_frequencies.items():
        group = "single" if len(word) == 1 else "multi"
        entries[(key, word)] = 5000 + round(4500 * math.log1p(frequency) / maxima[group])
    lines = ["# Modified by LocalPinyinIME contributors, 2026-10-12.",
             "# Source: Rime pinyin-simp, derived from Android Open Source Project PinyinIME.",
             f"# Upstream commit: {commit}; license: Apache-2.0; see THIRD_PARTY_NOTICES.txt.",
             "# Changes: short Han vocabulary filtering; joined syllables; logarithmic weight scaling; deduplication.",
             "# Existing project entries keep their direct-candidate score; this supplies only their decoding reference."]
    lines.extend(f"{key}\t{word}\t{score}" for (key, word), score in sorted(entries.items()))
    output = a.dictionary_dir / "standard_zh_pinyin.tsv"
    output.write_text("\n".join(lines)+"\n", encoding="utf-8", newline="\n")
    # Preserve original frequency evidence separately for authoring/calibration.
    (a.cache_dir / "standard_frequencies.json").write_text(json.dumps(
        [[key, word, frequency] for (key, word), frequency in sorted(all_frequencies.items())], ensure_ascii=False), encoding="utf-8", newline="\n")
    notices = a.dictionary_dir.parent / "THIRD_PARTY_NOTICES.txt"
    desktop_marker = "\n\nElectron desktop interface\n"
    previous_notices = notices.read_text(encoding="utf-8") if notices.exists() else ""
    desktop_notices = desktop_marker + previous_notices.split(desktop_marker, 1)[1] if desktop_marker in previous_notices else ""
    notices.write_text("Rime pinyin-simp / Android Open Source Project PinyinIME dictionary\n"
        f"https://github.com/{REPOSITORY}/tree/{commit}\n"
        "Original dictionary header: Derived from android open source project:\n"
        "http://android.git.kernel.org/?p=platform/packages/inputmethods/PinyinIME.git\n"
        "Upstream AUTHORS (the schema itself is not included in this product):\n" + authors_text.decode("utf-8") + "\n"
        "LocalPinyinIME modifications: filtered short Han words, normalized pinyin, scaled frequency weights, removed duplicates.\n"
        "Apache License 2.0 follows.\n\n"+license_text.decode("utf-8")+"\n"
        "\nCharacter context statistics\n"
        "The compact context model contains 1-4 character counts derived from original project-authored prose (MIT) and public-domain original prose by Lu Xun (1881-1936) and Zhu Ziqing (1898-1948).\n"
        "The full prose and Python authoring dependencies are not included in the installer.\n"
        "Source titles, authors, links, rights review and checksums: https://github.com/Terry1238832/LocalPinyinIME/blob/main/resources/dictionary/authoring/sources.json\n"
        "Model transformation and checksums: resources/dictionary/context_zh.metadata.json in the source repository.\n"
        "Validation works are excluded from training.\n"
        "\n\nNext-text continuations\n"
        "The bundled short next-text continuation pairs are original first-party text authored by LocalPinyinIME contributors and distributed under the project MIT license.\n"
        "Weights are relative editorial preferences. No corpus, commercial lexicon or user conversation history is imported.\n"
        "Authorship, review and checksums: resources/dictionary/authoring/continuations_sources.json and resources/dictionary/continuations_zh.metadata.json.\n"
        + desktop_notices, encoding="utf-8", newline="\n")
    metadata = {"format_version": 1, "source_repository": REPOSITORY, "upstream_commit": commit,
                "upstream_sha256": hashlib.sha256(raw).hexdigest(), "license": "Apache-2.0",
                "upstream_authors_sha256": hashlib.sha256(authors_text).hexdigest(),
                "entry_count": len(entries), "missing_entries_added": sum(key not in known for key in entries),
                "length_distribution": dict(sorted(Counter(len(word) for _, word in entries).items())),
                "sha256": hashlib.sha256(output.read_bytes()).hexdigest(), "skipped": dict(skipped),
                "heldout_sentences_excluded": True, "project_overrides_preserved": True,
                "weight_transform": "5000 + round(4500 * ln(1 + frequency) / max_ln); single/multi maxima calibrated separately",
                "normalization_max_ln": maxima}
    output.with_suffix(".metadata.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2)+"\n", encoding="utf-8", newline="\n")
    print(json.dumps(metadata, ensure_ascii=True))


if __name__ == "__main__":
    main()
