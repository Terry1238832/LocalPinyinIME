#!/usr/bin/env python3
"""Freeze independently authored short everyday vocabulary; never import lexica."""
from __future__ import annotations

from collections import Counter
import hashlib
import json
from pathlib import Path
import re

import pypinyin
from pypinyin import Style, lazy_pinyin, pinyin

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
FILES = [OUT / f"candidates_009_{scene}.txt" for scene in ("travel", "home", "campus")]
SCENES = {
    "校园社团与活动": "campus_clubs_and_events",
    "课堂安排与学生事务": "classes_and_student_services",
    "情绪沟通与相处": "emotional_communication",
    "电脑手机日常": "daily_phone_and_computer",
}
WEIGHTS = {
    "trip_planning_and_station": 5500,
    "ride_booking_and_airport": 5000,
    "lodging_and_guest_service": 5500,
    "travel_sightseeing_and_outings": 5000,
    "local_errands_and_public_space": 5500,
    "campus_clubs_and_events": 5000,
    "classes_and_student_services": 5000,
    "emotional_communication": 6000,
    "daily_phone_and_computer": 5500,
}

# Compound-level reviewed readings. Added after inspecting all surviving words,
# including multi-reading characters whose contextual default was already right.
OVERRIDES = {
    "调直椅背": "tiaozhiyibei",
    "座位调换": "zuoweidiaohuan",
    "宿舍调换申请": "sushediaohuanshenqing",
    "调课申请表": "diaokeshenqingbiao",
    "临时调课通知": "linshidiaoketongzhi",
    "过号重排": "guohaochongpai",
    "别落东西": "bieladongxi",
    "我得想一想": "wodeixiangyixiang",
    "蓝牙耳机重置": "lanyaerjichongzhi",
    "文件路径太长": "wenjianlujingtaichang",
    "长截图失败": "changjietushibai",
    "移动硬盘弹出": "yidongyingpantanchu",
    "无线网络重连": "wuxianwangluochonglian",
    "网页弹窗拦截": "wangyetanchuanglanjie",
    "通讯录去重": "tongxunluquchong",
    "长靴支撑架": "changxuezhichengjia",
    "长柄鞋拔": "changbingxieba",
    "垃圾袋扎好": "lajidaizahao",
    "够不着上面": "goubuzhaoshangmian",
    "螺丝太长": "luositaichang",
    "密封垫圈": "mifengdianquan",
    "橡胶垫圈": "xiangjiaodianquan",
    "换个垫圈": "huangedianquan",
    "洗衣机调平": "xiyijitiaoping",
    "热水器点不着": "reshuiqidianbuzhao",
    "燃气灶打不着": "ranqizaodabuzhao",
    "调个蘸汁": "tiaogezhanzhi",
    "发票重开": "fapiaochongkai",
    "买重了": "maichongle",
    "还没收到货": "haimeishoudaohuo",
    "取件码没收到": "qujianmameishoudao",
}
POLYPHONES = "行长重调假还得藏薄处校系载便单转觉乐宿量数供应间差弹传解着难干圈缝会空省发划钻泊铺屈强夹纶粘兴好拗露辟落翘折角率卷鲜降参盛秘斗闷蒙漂踏纤蚕蕃质颈杆壳舍脏脯咽咳骨累识刨剥拓卡扇扁冠熨择给荷腊屏尺混查没朝禁"
PRONUNCIATION_REFERENCE_LINKS = {
    "调换": "https://dict.revised.moe.edu.tw/dictView.jsp?ID=46020&la=0&powerMode=0",
    "调课": "https://dict.revised.moe.edu.tw/dictView.jsp?ID=46011&la=0&powerMode=0",
    "落下（遗落）": "https://dict.revised.moe.edu.tw/dictView.jsp?ID=59225&la=0&powerMode=0",
    "重置价格": "https://dict.revised.moe.edu.tw/dictView.jsp?ID=120506&la=0&powerMode=0",
}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def candidates():
    for path in FILES:
        category = ""
        for line in path.read_text(encoding="utf-8").splitlines():
            if line.startswith("# category:"):
                category = line.partition(":")[2].strip()
            elif line.startswith("# scene:"):
                scene = line.partition(":")[2].strip()
                category = SCENES.get(scene, scene)
            elif line and not line.startswith("#"):
                assert category, f"Missing scene in {path}"
                for word in line.split():
                    assert re.fullmatch(r"[\u4e00-\u9fff]{2,6}", word), word
                    yield category, word, WEIGHTS.get(category, 5500)


def main() -> None:
    assert pypinyin.__version__ == "0.55.0"
    if (OUT / "lexicon_batch_009.applied.json").exists():
        raise SystemExit("Batch 009 is applied; retain its frozen authoring baselines.")
    base = ROOT / "resources/dictionary"
    baselines = [base / name for name in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv")]
    baselines += [OUT / f"lexicon_batch_{n:03d}.tsv" for n in range(1, 9)]
    known_keys, known_words = set(), set()
    for path in baselines:
        for line in path.read_text(encoding="utf-8-sig").splitlines():
            if line and not line.startswith("#"):
                key, word, _ = line.split("\t")
                known_keys.add((key, word))
                known_words.add(word)
    benchmarks = sorted((ROOT / "tests/data").glob("typing_*.tsv"))
    answers = {line.split("\t")[1] for path in benchmarks for line in path.read_text(encoding="utf-8-sig").splitlines()
               if line and not line.startswith("#")}
    selected, seen, alternate, excluded_benchmark = [], set(), [], []
    authored = 0
    for category, word, weight in candidates():
        authored += 1
        if word in seen:
            continue
        seen.add(word)
        if word in answers:
            excluded_benchmark.append(word)
            continue
        key = OVERRIDES.get(word) or "".join(lazy_pinyin(word, style=Style.NORMAL, strict=True))
        assert re.fullmatch("[a-z]+", key), (key, word)
        if (key, word) in known_keys:
            continue
        if word in known_words:
            alternate.append({"word": word, "proposed_pinyin": key})
            continue
        selected.append((category, key, word, weight))
    selected.sort(key=lambda r: (r[0], r[1], r[2]))
    assert len(selected) == len({(k, w) for _, k, w, _ in selected})
    for _, key, word, weight in selected:
        positions = {0}
        for char in word:
            choices = pinyin(char, style=Style.NORMAL, heteronym=True, strict=True)[0]
            positions = {i + len(c) for i in positions for c in choices if key.startswith(c, i)}
        assert len(key) in positions, ("Impossible character reading", key, word)
        assert (key, word) not in known_keys and word not in known_words and word not in answers
    lines, previous = [], None
    for category, key, word, weight in selected:
        if previous != category:
            lines.append(f"# category: {category}")
            previous = category
        lines.append(f"{key}\t{word}\t{weight}")
    output = OUT / "lexicon_batch_009.tsv"
    output.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    selected_words = {r[2] for r in selected}
    metadata = {
        "batch": "lexicon_batch_009", "authoring_source": "first_party_authored", "external_lexicon_import": False,
        "license": "MIT", "pronunciation_tool": "pypinyin 0.55.0 authoring-only; reviewed compound overrides",
        "authoring_files_sha256": {p.name: sha(p) for p in FILES},
        "authored_candidates": authored, "new_entries": len(selected), "unique_words": len(selected_words),
        "sha256": sha(output), "category_counts": dict(sorted(Counter(r[0] for r in selected).items())),
        "length_distribution": dict(sorted(Counter(str(len(r[2])) for r in selected).items())),
        "frequency_range": sorted({r[3] for r in selected}),
        "frequency_policy": "Coarse project-relative 5000/5500/6000 bands by everyday utility; not corpus statistics or an instruction to force first place.",
        "dedup_policy": "Exclude existing key/display pair and all existing display words across core, local_core, standard and all previous frozen batches",
        "dedup_baseline_sha256": {p.name: sha(p) for p in baselines}, "benchmark_sha256": {p.name: sha(p) for p in benchmarks},
        "benchmark_full_answers_added": 0, "benchmark_answers_excluded": excluded_benchmark,
        "alternate_pronunciation_candidates_excluded": alternate,
        "manual_pronunciation_overrides": {w: k for w, k in OVERRIDES.items() if w in selected_words},
        "pronunciation_review_reference_links": PRONUNCIATION_REFERENCE_LINKS,
        "pronunciation_reference_scope": "Read individual word pronunciation definitions only; no external lexicon, definition text, or usage example was imported.",
        "validation": {"all_outputs_two_to_six_han": True, "format_valid": True,
            "per_character_possible_readings_valid": True, "canonical_or_previous_batch_collisions": 0,
            "automatic_default_readings_corrected": sum(k != "".join(lazy_pinyin(w, style=Style.NORMAL, strict=True))
                for w, k in OVERRIDES.items() if w in selected_words),
            "explicit_override_keys": sum(w in selected_words for w in OVERRIDES),
            "polyphone_reviewed_display_words": sum(any(c in w for c in POLYPHONES) for _, _, w, _ in selected)},
        "canonical_dictionaries_modified": False, "line_endings": "UTF-8 LF",
    }
    output.with_suffix(".json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({k: metadata[k] for k in ("new_entries", "sha256", "category_counts", "length_distribution")}, ensure_ascii=False))


if __name__ == "__main__":
    main()
