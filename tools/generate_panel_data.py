#!/usr/bin/env python3
"""Generate the tool panel's symbol, emoji and kaomoji data.

Usage:
  python tools/generate_panel_data.py --emoji-test emoji-test.txt \
      --annotations annotations.json --derived annotations-derived.json \
      --kaomoji emoticons.json

Sources (see resources/THIRD_PARTY_NOTICES.txt):
  emoji-test.txt   https://unicode.org/Public/emoji/latest/emoji-test.txt (Unicode License v3)
  annotations      https://github.com/unicode-org/cldr-json  cldr-annotations-full/annotations/zh
                   and cldr-annotations-derived-full/annotationsDerived/zh (Unicode License v3)
  emoticons.json   https://github.com/w33ble/emoticon-data (MIT, (c) 2014 Joe Fleming)

Symbols come from this Python's Unicode database; nothing is downloaded.
Writes desktop/src/panel/data/{symbols,emoji,kaomoji}.json for the panel
window and resources/dictionary/panel/{symbols,emoji,kaomoji}.tsv for the
input method's own small panel: one item per line, group id, group name,
section, text, name and keywords (joined by |).
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import unicodedata

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "desktop" / "src" / "panel" / "data"
NATIVE_OUT = ROOT / "resources" / "dictionary" / "panel"

# Windows 11's Segoe UI Emoji draws these; newer emoji would show as boxes.
MAX_EMOJI_VERSION = 15.0
SKIN_TONES = {0x1F3FB, 0x1F3FC, 0x1F3FD, 0x1F3FE, 0x1F3FF}
EMOJI_GROUPS = {
    "Smileys & Emotion": ("smileys", "笑脸与情感"),
    "People & Body": ("people", "人物与身体"),
    "Animals & Nature": ("nature", "动物与自然"),
    "Food & Drink": ("food", "食物与饮料"),
    "Travel & Places": ("travel", "旅行与地点"),
    "Activities": ("activities", "活动"),
    "Objects": ("objects", "物品"),
    "Symbols": ("symbols", "符号"),
    "Flags": ("flags", "旗帜"),
}

# Kaomoji: each goes to the first of these its tags name.
KAOMOJI_GROUPS = [
    ("happy", "开心", ["happy", "excited", "fun", "laugh", "wink"]),
    ("love", "喜欢", ["love", "kiss", "hug"]),
    ("sad", "难过", ["sad", "cry", "hurt"]),
    ("angry", "生气", ["angry", "table flip", "evil"]),
    ("surprised", "惊讶", ["surprised", "scared", "worried"]),
    ("shy", "害羞", ["embarassed", "hide"]),
    ("speechless", "无语", ["whatever", "confused", "stare", "smug", "troll", "think", "sorry", "surrender", "dead",
                           "crazy"]),
    ("greeting", "打招呼", ["wave"]),
    ("dance", "跳舞唱歌", ["dance", "music"]),
    ("sleep", "睡觉", ["sleep"]),
    ("animals", "动物", ["animal", "cat", "bear", "rabbit", "bird", "sea creature", "dog", "pig", "monkey"]),
    ("action", "动作", ["run", "write"]),
]
KAOMOJI_TARGET = 1000

# Symbols with Chinese names; the rest are named from Unicode.
COMMON_MATH = [
    ("+", "加号"), ("−", "减号"), ("×", "乘号"), ("÷", "除号"), ("±", "正负号"), ("∓", "负正号"), ("=", "等号"),
    ("≠", "不等于"), ("≈", "约等于"), ("≡", "恒等于"), ("<", "小于"), (">", "大于"), ("≤", "小于等于"), ("≥", "大于等于"),
    ("≪", "远小于"), ("≫", "远大于"), ("∝", "正比于"), ("∞", "无穷大"), ("√", "根号"), ("∛", "立方根"), ("∜", "四次方根"),
    ("%", "百分号"), ("‰", "千分号"), ("‱", "万分号"), ("°", "度"), ("′", "分"), ("″", "秒"), ("℃", "摄氏度"), ("℉", "华氏度"),
    ("π", "圆周率"), ("∑", "求和"), ("∏", "求积"), ("∫", "积分"), ("∬", "二重积分"), ("∮", "环路积分"), ("∂", "偏导"),
    ("∇", "梯度"), ("Δ", "增量"), ("∠", "角"), ("⊥", "垂直"), ("∥", "平行"), ("∽", "相似"), ("≌", "全等"), ("⌒", "弧"),
    ("⊙", "圆"), ("△", "三角形"), ("∴", "所以"), ("∵", "因为"), ("∈", "属于"), ("∉", "不属于"), ("⊂", "真子集"),
    ("⊃", "真包含"), ("⊆", "子集"), ("⊇", "包含"), ("∪", "并集"), ("∩", "交集"), ("∅", "空集"), ("∀", "任意"), ("∃", "存在"),
    ("¬", "非"), ("∧", "且"), ("∨", "或"), ("⇒", "推出"), ("⇔", "等价"), ("→", "趋向"), ("↦", "映射"),
    ("¹", "上标 1"), ("²", "平方"), ("³", "立方"), ("⁴", "上标 4"), ("ⁿ", "上标 n"), ("⁻", "上标负号"), ("₀", "下标 0"),
    ("₁", "下标 1"), ("₂", "下标 2"), ("₃", "下标 3"), ("ₙ", "下标 n"), ("½", "二分之一"), ("⅓", "三分之一"),
    ("⅔", "三分之二"), ("¼", "四分之一"), ("¾", "四分之三"), ("⅕", "五分之一"), ("ℕ", "自然数集"), ("ℤ", "整数集"),
    ("ℚ", "有理数集"), ("ℝ", "实数集"), ("ℂ", "复数集"), ("α", "阿尔法"), ("β", "贝塔"), ("γ", "伽马"), ("θ", "西塔"),
    ("λ", "兰姆达"), ("μ", "缪"), ("σ", "西格玛"), ("φ", "斐"), ("ω", "欧米伽"),
]
PUNCTUATION = [
    ("中文标点", "，。、；：？！“”‘’（）【】《》〈〉「」『』〔〕〖〗…—～·．＿＃＆＊＠＼／｜"),
    ("英文标点", ",.;:?!\"'()[]{}<>-_/\\|@#&*^`~"),
    ("其他标点", "•‧※§¶†‡‖′″‴‹›«»¡¿–—―‥‣⁂⁎⁑"),
]
CURRENCY_EXTRA = "$¢£¤¥֏؋৲৳฿៛﷼＄￠￡￥￦"
PINYIN_LETTERS = "āáǎàōóǒòēéěèīíǐìūúǔùǖǘǚǜüÜêÊńňǹḿ"


def assigned(code: int) -> bool:
    ch = chr(code)
    if unicodedata.category(ch) in {"Mn", "Me", "Cc", "Cf", "Cs", "Co", "Cn", "Zs", "Zl", "Zp"}:
        return False
    try:
        unicodedata.name(ch)
    except ValueError:
        return False
    return True


def named(text: str) -> str:
    try:
        return unicodedata.name(text).lower() if len(text) == 1 else ""
    except ValueError:
        return ""


def span(start: int, end: int) -> list[str]:
    return [chr(code) for code in range(start, end + 1) if assigned(code)]


# Chinese names for what CLDR does not name: Chinese punctuation, Greek
# letters, pinyin tones and numbered forms. Search also finds the CLDR
# keywords and the English Unicode name.
PUNCTUATION_NAMES = {
    "，": "逗号", "。": "句号", "、": "顿号", "；": "分号", "：": "冒号", "？": "问号", "！": "感叹号", "“": "左双引号",
    "”": "右双引号", "‘": "左单引号", "’": "右单引号", "（": "左括号", "）": "右括号", "【": "左方头括号", "】": "右方头括号",
    "《": "左书名号", "》": "右书名号", "〈": "左单书名号", "〉": "右单书名号", "「": "左直角引号", "」": "右直角引号",
    "『": "左双直角引号", "』": "右双直角引号", "〔": "左龟甲括号", "〕": "右龟甲括号", "〖": "左空心方头括号", "〗": "右空心方头括号",
    "…": "省略号", "—": "破折号", "～": "波浪号", "·": "间隔号", "．": "全角点", "＿": "全角下划线", "＃": "全角井号",
    "＆": "全角和号", "＊": "全角星号", "＠": "全角艾特", "＼": "全角反斜杠", "／": "全角斜杠", "｜": "全角竖线",
    ",": "英文逗号", ".": "英文句号", ";": "英文分号", ":": "英文冒号", "?": "英文问号", "!": "英文感叹号", "\"": "英文双引号",
    "'": "英文单引号", "(": "英文左括号", ")": "英文右括号", "[": "左方括号", "]": "右方括号", "{": "左花括号", "}": "右花括号",
    "<": "小于号", ">": "大于号", "-": "连字符", "_": "下划线", "/": "斜杠", "\\": "反斜杠", "|": "竖线", "@": "艾特",
    "#": "井号", "&": "和号", "*": "星号", "^": "脱字符", "`": "反引号", "~": "波浪号", "•": "圆点", "※": "参考符号",
    "¶": "段落符号", "†": "剑号", "‡": "双剑号", "‖": "双竖线", "«": "左书名号", "»": "右书名号", "¡": "倒感叹号", "¿": "倒问号",
    "–": "短破折号", "―": "横线", "‥": "两点省略号", "★": "实心星", "☆": "空心星",
}
GREEK = dict(zip("ALPHA BETA GAMMA DELTA EPSILON ZETA ETA THETA IOTA KAPPA LAMDA MU NU XI OMICRON PI RHO SIGMA TAU "
                 "UPSILON PHI CHI PSI OMEGA".split(),
                 ["阿尔法", "贝塔", "伽马", "德尔塔", "艾普西隆", "泽塔", "伊塔", "西塔", "约塔", "卡帕", "兰姆达", "缪", "纽",
                  "克西", "奥密克戎", "派", "柔", "西格玛", "陶", "宇普西隆", "斐", "希", "普西", "欧米伽"]))
TONES = {"MACRON": "一声", "ACUTE": "二声", "CARON": "三声", "GRAVE": "四声"}
CHINESE_DIGITS = "零一二三四五六七八九十"


def chinese_number(value: int) -> str:
    if value <= 10:
        return CHINESE_DIGITS[value]
    tens, ones = divmod(value, 10)
    return ("" if tens == 1 else CHINESE_DIGITS[tens]) + "十" + (CHINESE_DIGITS[ones] if ones else "")


def rule_name(ch: str) -> str:
    english = named(ch).upper()
    greek = re.fullmatch(r"GREEK (CAPITAL|SMALL) LETTER (FINAL )?(\w+)", english)
    if greek and greek.group(3) in GREEK:
        return ("大写" if greek.group(1) == "CAPITAL" else "小写") + ("词尾" if greek.group(2) else "") + GREEK[greek.group(3)]
    if english.startswith("LATIN") and ("SMALL LETTER" in english or "CAPITAL LETTER" in english):
        letter = english.split(" LETTER ", 1)[1].split()[0]
        for mark, tone in TONES.items():
            if f"WITH {mark}" in english and len(letter) == 1:
                return f"拼音 {letter.lower() if 'SMALL' in english else letter}{'ü' if 'DIAERESIS' in english else ''} {tone}"
    number = unicodedata.numeric(ch, -1)
    if number == int(number) and 0 < number <= 50:
        value = int(number)
        if english.startswith("CIRCLED") or english.startswith("NEGATIVE CIRCLED") or english.startswith("DOUBLE CIRCLED"):
            return f"带圈数字{value}"
        if english.startswith("PARENTHESIZED"):
            return f"带括号数字{value}"
        if english.endswith("FULL STOP"):
            return f"带点数字{value}"
        if english.startswith("ROMAN NUMERAL"):
            return f"{'小写' if 'SMALL' in english else ''}罗马数字{chinese_number(value)}"
    if english.startswith("CIRCLED LATIN"):
        return "带圈大写字母 " + english.split()[-1] if "CAPITAL" in english else "带圈小写字母 " + english.split()[-1].lower()
    if english.startswith("CIRCLED IDEOGRAPH") or english.startswith("PARENTHESIZED IDEOGRAPH"):
        return ("带圈" if english.startswith("CIRCLED") else "带括号") + "汉字"
    return ""


class Names:
    def __init__(self, annotations: dict):
        self.annotations = annotations

    def item(self, ch: str, chinese: str | None = None) -> dict:
        annotation = self.annotations.get(ch) or {}
        name = chinese or PUNCTUATION_NAMES.get(ch) or (annotation.get("tts") or [""])[0] or rule_name(ch) or named(ch)
        keywords = [word for word in annotation.get("default", []) if word != name][:5]
        english = named(ch)
        if english and english != name:
            keywords.append(english)
        return {"c": ch, "n": name, "k": keywords} if keywords else {"c": ch, "n": name}


def section(name: str, characters, names: Names, chinese: dict[str, str] | None = None) -> dict:
    seen, items = set(), []
    for ch in characters:
        if ch in seen:
            continue
        seen.add(ch)
        items.append(names.item(ch, (chinese or {}).get(ch)))
    return {"name": name, "items": items}


def symbols(annotations: dict) -> dict:
    names = Names(annotations)
    math_names = dict(COMMON_MATH)
    groups = [
        {"id": "common-math", "name": "常用数学符号",
         "sections": [section("常用", [c for c, _ in COMMON_MATH], names, math_names)]},
        {"id": "punctuation", "name": "常规标点", "sections": [section(name, chars, names) for name, chars in PUNCTUATION]},
        {"id": "currency", "name": "货币符号",
         "sections": [section("常用货币", "¥$€£¢₩₹₽₺₿₫₪₱₦₴₸₼₾฿", names),
                      section("全部货币", list(CURRENCY_EXTRA) + span(0x20A0, 0x20C0), names)]},
        {"id": "latin", "name": "拉丁符号",
         "sections": [section("拼音声调", PINYIN_LETTERS, names),
                      section("带重音字母", span(0x00C0, 0x00FF), names),
                      section("扩展拉丁 A", span(0x0100, 0x017F), names),
                      section("扩展拉丁 B", span(0x0180, 0x024F), names),
                      section("越南语与附加字母", span(0x1E00, 0x1EFF), names)]},
        {"id": "math", "name": "数学符号",
         "sections": [section("数学运算符", span(0x2200, 0x22FF), names),
                      section("其他数学符号", span(0x27C0, 0x27EF) + span(0x2980, 0x29FF), names)]},
        {"id": "greek", "name": "希腊字母",
         "sections": [section("大写", span(0x0391, 0x03A9), names), section("小写", span(0x03B1, 0x03C9), names),
                      section("变体", "ϐϑϒϕϖϰϱϵ", names)]},
        {"id": "arrows", "name": "箭头符号",
         "sections": [section("箭头", span(0x2190, 0x21FF), names), section("长箭头", span(0x27F0, 0x27FF), names),
                      section("更多箭头", span(0x2900, 0x297F) + span(0x2B00, 0x2B11), names)]},
        {"id": "geometry", "name": "几何符号",
         "sections": [section("几何图形", span(0x25A0, 0x25FF), names),
                      section("更多图形", span(0x2B1B, 0x2B2F) + span(0x2B50, 0x2B59), names)]},
        {"id": "other", "name": "其他 Unicode 特殊字符",
         "sections": [section("类字母符号", span(0x2100, 0x214F), names),
                      section("数字形式", span(0x2150, 0x218B), names),
                      section("带圈字母数字", span(0x2460, 0x24FF), names),
                      section("带圈中文", span(0x3220, 0x3247) + span(0x3280, 0x32B0), names),
                      section("杂项技术符号", span(0x2300, 0x23FF), names),
                      section("杂项符号", span(0x2600, 0x26FF), names),
                      section("装饰符号", span(0x2700, 0x27BF), names),
                      section("制表符", span(0x2500, 0x259F), names),
                      section("中日韩符号", span(0x3001, 0x303F), names)]},
    ]
    for group in groups:
        group["sections"] = [s for s in group["sections"] if s["items"]]
    return {"groups": groups}


def emoji(test: Path, annotations: Path, derived: Path) -> dict:
    names = json.loads(annotations.read_text(encoding="utf-8"))["annotations"]["annotations"]
    names.update(json.loads(derived.read_text(encoding="utf-8"))["annotationsDerived"]["annotations"])
    groups: list[dict] = []
    current = None
    line_pattern = re.compile(r"^([0-9A-F ]+?)\s*;\s*fully-qualified\s*#\s*\S+\s+E(\d+\.\d+)\s")
    for line in test.read_text(encoding="utf-8").splitlines():
        if line.startswith("# group:"):
            title = line.split(":", 1)[1].strip()
            current = None
            if title in EMOJI_GROUPS:
                gid, gname = EMOJI_GROUPS[title]
                current = {"id": gid, "name": gname, "items": []}
                groups.append(current)
            continue
        match = line_pattern.match(line)
        if not match or current is None:
            continue
        codes = [int(part, 16) for part in match.group(1).split()]
        if float(match.group(2)) > MAX_EMOJI_VERSION or any(code in SKIN_TONES for code in codes):
            continue
        text = "".join(chr(code) for code in codes)
        annotation = names.get(text) or names.get(text.replace("️", "")) or {}
        name = (annotation.get("tts") or [""])[0]
        keywords = [k for k in annotation.get("default", []) if k != name][:6]
        current["items"].append({"c": text, "n": name, "k": keywords})
    return {"groups": groups}


def kaomoji(source: Path) -> dict:
    data = json.loads(source.read_text(encoding="utf-8"))["emoticons"]
    buckets: dict[str, list[str]] = {gid: [] for gid, _, _ in KAOMOJI_GROUPS}
    seen = set()
    for entry in data:
        text = entry["string"].strip()
        tags = set(entry.get("tags", []))
        if not text or text in seen or len(text) > 20 or any(unicodedata.category(c) in {"Cc", "Cf"} for c in text):
            continue
        for gid, _, wanted in KAOMOJI_GROUPS:
            if tags & set(wanted):
                buckets[gid].append(text)
                seen.add(text)
                break
    total = sum(len(items) for items in buckets.values())
    # About KAOMOJI_TARGET in all, each mood keeping its share.
    groups = []
    for gid, name, _ in KAOMOJI_GROUPS:
        items = buckets[gid]
        keep = max(1, round(len(items) * KAOMOJI_TARGET / total)) if total > KAOMOJI_TARGET else len(items)
        groups.append({"id": gid, "name": name, "items": items[:keep]})
    return {"groups": groups}


def tsv(value: dict) -> str:
    def clean(text: str) -> str:
        return text.replace("\t", " ").replace("\n", " ")
    lines = []
    for group in value["groups"]:
        sections = group.get("sections") or [{"name": "", "items": group["items"]}]
        for section in sections:
            for item in section["items"]:
                entry = item if isinstance(item, dict) else {"c": item, "n": group["name"]}
                keywords = "|".join(clean(word) for word in entry.get("k", []) if "|" not in word)
                lines.append("\t".join([group["id"], group["name"], clean(section.get("name", "")), clean(entry["c"]),
                                        clean(entry.get("n", "")), keywords]))
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--emoji-test", type=Path, required=True)
    parser.add_argument("--annotations", type=Path, required=True)
    parser.add_argument("--derived", type=Path, required=True)
    parser.add_argument("--kaomoji", type=Path, required=True)
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    NATIVE_OUT.mkdir(parents=True, exist_ok=True)
    outputs = {
        "symbols.json": symbols(json.loads(args.annotations.read_text(encoding="utf-8"))["annotations"]["annotations"]),
        "emoji.json": emoji(args.emoji_test, args.annotations, args.derived),
        "kaomoji.json": kaomoji(args.kaomoji),
    }
    for name, value in outputs.items():
        path = OUT / name
        path.write_text(json.dumps(value, ensure_ascii=False, separators=(",", ":")) + "\n", encoding="utf-8")
        (NATIVE_OUT / name.replace(".json", ".tsv")).write_text(tsv(value), encoding="utf-8", newline="\n")
        counts = [len(g.get("items", [])) or sum(len(s["items"]) for s in g.get("sections", [])) for g in value["groups"]]
        print(f"{name}: {path.stat().st_size} bytes, {sum(counts)} items in {len(counts)} groups {counts}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
