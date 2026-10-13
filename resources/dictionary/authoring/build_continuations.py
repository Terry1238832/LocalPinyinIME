#!/usr/bin/env python3
"""Build explicitly authored short next-text suggestions; no runtime Python."""
from __future__ import annotations
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
BASE = ROOT / "resources/dictionary"

# Each prefix and its alternatives are individually authored. Weights express
# practical author preference, not corpus frequencies or learned user history.
GROUPS = [
    ("daily_plans", "我想", [("去", 9000), ("看看", 8600), ("试试", 8200), ("问一下", 7800)]),
    ("daily_plans", "我想去", [("看看", 8500), ("散散步", 7800), ("买点东西", 7200)]),
    ("daily_plans", "我想吃", [("点东西", 8500), ("点水果", 7600), ("面条", 6800)]),
    ("daily_plans", "我想喝", [("点水", 8600), ("杯茶", 7800), ("点热水", 7200)]),
    ("daily_plans", "我准备", [("出门", 8600), ("回家", 7900), ("休息一下", 7400)]),
    ("daily_plans", "我打算", [("先看看", 8500), ("试一下", 7900), ("出去走走", 7400)]),
    ("daily_plans", "我需要", [("确认一下", 8600), ("一点时间", 8100), ("你的帮助", 7500)]),
    ("daily_plans", "我希望", [("能顺利完成", 8500), ("可以试试", 7800), ("大家都好", 7400)]),
    ("daily_plans", "我们可以", [("先讨论一下", 8500), ("一起去", 8100), ("再看看", 7600)]),
    ("daily_plans", "我们先", [("确认一下", 8500), ("商量一下", 8100), ("看看情况", 7600)]),
    ("weather_and_state", "今天天气", [("不错", 9000), ("很好", 8500), ("有点冷", 7800), ("有点热", 7300)]),
    ("weather_and_state", "明天天气", [("怎么样", 8800), ("可能会变", 7400), ("应该不错", 7000)]),
    ("weather_and_state", "外面", [("有点冷", 8500), ("在下雨", 8000), ("风挺大", 7500)]),
    ("weather_and_state", "今天有点", [("忙", 8800), ("累", 8200), ("冷", 7600)]),
    ("weather_and_state", "最近有点", [("忙", 8800), ("累", 8200), ("烦", 7200)]),
    ("weather_and_state", "今天心情", [("不错", 8800), ("很好", 8200), ("有点低落", 7300)]),
    ("weather_and_state", "最近感觉", [("还不错", 8600), ("有点累", 8100), ("好多了", 7600)]),
    ("weather_and_state", "看起来", [("不错", 8700), ("挺好的", 8100), ("很合适", 7600)]),
    ("weather_and_state", "感觉", [("还不错", 8600), ("挺好的", 8100), ("好多了", 7600)]),
    ("weather_and_state", "有点", [("担心", 8500), ("累", 8100), ("不太确定", 7600)]),
    ("care_and_politeness", "辛苦", [("了", 9600), ("你了", 8900), ("大家了", 8300)]),
    ("care_and_politeness", "谢谢", [("你", 9300), ("提醒", 8700), ("帮忙", 8200)]),
    ("care_and_politeness", "感谢", [("你的帮助", 8800), ("你的理解", 8300), ("大家的支持", 7800)]),
    ("care_and_politeness", "不好意思", [("打扰一下", 8700), ("让你久等了", 8200), ("我看错了", 7700)]),
    ("care_and_politeness", "麻烦", [("你了", 9000), ("帮忙看看", 8400), ("确认一下", 7900)]),
    ("care_and_politeness", "请帮我", [("确认一下", 8700), ("找一下", 8200), ("看一下", 7700)]),
    ("care_and_politeness", "请帮忙", [("确认", 8800), ("处理", 8200), ("查收", 7700)]),
    ("care_and_politeness", "请注意", [("安全", 9000), ("休息", 8500), ("保暖", 8000)]),
    ("care_and_politeness", "记得", [("吃饭", 8900), ("喝水", 8400), ("休息", 7900)]),
    ("care_and_politeness", "早点", [("休息", 9200), ("睡觉", 8600), ("回来", 7900)]),
    ("care_and_politeness", "路上", [("注意安全", 9100), ("小心", 8500), ("慢点走", 7800)]),
    ("care_and_politeness", "到家", [("告诉我", 8700), ("说一声", 8200), ("报个平安", 7600)]),
    ("care_and_politeness", "不用", [("担心", 8800), ("客气", 8300), ("着急", 7800)]),
    ("care_and_politeness", "别太", [("担心", 8800), ("辛苦", 8300), ("着急", 7800)]),
    ("chat_and_replies", "收到", [("了", 9300), ("谢谢", 8600), ("我看一下", 7900)]),
    ("chat_and_replies", "知道", [("了", 9400), ("啦", 8200), ("你的意思了", 7600)]),
    ("chat_and_replies", "明白", [("了", 9300), ("你的意思了", 8100), ("谢谢提醒", 7500)]),
    ("chat_and_replies", "好的", [("没问题", 8900), ("谢谢", 8500), ("我确认一下", 7900)]),
    ("chat_and_replies", "可以", [("试试", 8800), ("先看看", 8300), ("商量一下", 7800)]),
    ("chat_and_replies", "没问题", [("我来处理", 8500), ("稍后回复", 7900), ("放心吧", 7400)]),
    ("chat_and_replies", "有空", [("聊聊", 8600), ("再回复", 8100), ("一起去", 7600)]),
    ("chat_and_replies", "有时间", [("再看看", 8500), ("一起聊聊", 7900), ("告诉我", 7400)]),
    ("chat_and_replies", "如果方便", [("请回复一下", 8800), ("帮忙看看", 8100), ("一起去", 7500)]),
    ("chat_and_replies", "等你", [("回复", 8700), ("消息", 8200), ("确认", 7700)]),
    ("chat_and_replies", "最近", [("怎么样", 8600), ("还好吗", 8200), ("有点忙", 7700)]),
    ("chat_and_replies", "好久不见", [("最近怎么样", 8500), ("有空聊聊", 7900), ("改天再聚", 7400)]),
    ("chat_and_replies", "祝你", [("顺利", 9000), ("开心", 8500), ("好运", 8100)]),
    ("chat_and_replies", "周末", [("愉快", 9000), ("一起去", 8000), ("有空吗", 7500)]),
    ("office_and_documents", "请查收", [("附件", 9200), ("邮件", 8500), ("资料", 7900)]),
    ("office_and_documents", "请查看", [("附件", 9000), ("最新版本", 8300), ("具体内容", 7700)]),
    ("office_and_documents", "请确认", [("时间", 8800), ("收到", 8300), ("具体安排", 7800)]),
    ("office_and_documents", "请核对", [("信息", 8900), ("数据", 8300), ("名单", 7800)]),
    ("office_and_documents", "请提供", [("资料", 8900), ("相关信息", 8300), ("具体说明", 7700)]),
    ("office_and_documents", "请反馈", [("意见", 9000), ("进度", 8400), ("处理结果", 7800)]),
    ("office_and_documents", "附件", [("请查收", 9000), ("已更新", 8300), ("如下", 7700)]),
    ("office_and_documents", "文件", [("已发送", 8800), ("已更新", 8200), ("请查收", 7700)]),
    ("office_and_documents", "资料", [("已整理好", 8800), ("已发送", 8200), ("请查收", 7700)]),
    ("office_and_documents", "邮件", [("已发送", 8800), ("已收到", 8200), ("请查收", 7700)]),
    ("office_and_documents", "如有", [("疑问", 9000), ("问题", 8500), ("变更", 7800)]),
    ("office_and_documents", "如需", [("补充", 8900), ("调整", 8400), ("协助", 7800)]),
    ("office_and_documents", "以附件", [("为准", 9100), ("内容为准", 8300)]),
    ("office_and_documents", "以通知", [("为准", 9100), ("内容为准", 8300)]),
    ("work_and_progress", "下一步", [("安排", 8800), ("计划", 8300), ("再确认", 7800)]),
    ("work_and_progress", "接下来", [("再讨论", 8700), ("继续完善", 8100), ("确认一下", 7600)]),
    ("work_and_progress", "后续", [("再沟通", 8600), ("继续跟进", 8100), ("安排", 7600)]),
    ("work_and_progress", "我们需要", [("确认一下", 8800), ("进一步沟通", 8200), ("提前准备", 7700)]),
    ("work_and_progress", "需要", [("确认一下", 8700), ("补充说明", 8100), ("进一步核实", 7600)]),
    ("work_and_progress", "正在", [("处理", 8900), ("准备", 8300), ("核对", 7800)]),
    ("work_and_progress", "已经", [("完成", 8900), ("处理好了", 8300), ("确认", 7800)]),
    ("work_and_progress", "尚未", [("确认", 8900), ("完成", 8300), ("收到", 7800)]),
    ("work_and_progress", "还需要", [("一点时间", 8800), ("确认一下", 8200), ("补充材料", 7600)]),
    ("work_and_progress", "初步", [("方案", 8700), ("计划", 8200), ("意见", 7600)]),
    ("work_and_progress", "进一步", [("讨论", 8700), ("确认", 8200), ("完善", 7700)]),
    ("work_and_progress", "有待", [("确认", 8800), ("完善", 8200), ("解决", 7700)]),
    ("work_and_progress", "先确认", [("时间", 8900), ("需求", 8300), ("具体情况", 7800)]),
    ("work_and_progress", "继续", [("跟进", 8800), ("完善", 8200), ("推进", 7700)]),
    ("time_and_meetings", "明天", [("再说", 8600), ("联系", 8000), ("见", 7500)]),
    ("time_and_meetings", "下次", [("再说", 8800), ("一起去", 8200), ("再聊", 7600)]),
    ("time_and_meetings", "改天", [("再聊", 8800), ("一起吃饭", 8200), ("再约", 7600)]),
    ("time_and_meetings", "稍后", [("回复", 8900), ("联系", 8300), ("再说", 7800)]),
    ("time_and_meetings", "晚点", [("联系", 8800), ("再说", 8200), ("回复", 7700)]),
    ("time_and_meetings", "等一下", [("我确认一下", 8600), ("马上就好", 8100), ("再说", 7400)]),
    ("time_and_meetings", "忙完", [("再说", 8900), ("联系你", 8200), ("告诉我", 7600)]),
    ("time_and_meetings", "有空的时候", [("再看看", 8500), ("联系我", 8000), ("回复一下", 7500)]),
    ("time_and_meetings", "方便的话", [("联系我", 8700), ("告诉我", 8200), ("回复一下", 7600)]),
    ("time_and_meetings", "确定", [("时间", 8800), ("以后告诉我", 8100), ("后再说", 7500)]),
    ("time_and_meetings", "约个", [("时间", 9000), ("地方", 8300)]),
    ("time_and_meetings", "找个", [("时间", 8800), ("地方", 8200), ("机会", 7700)]),
    ("shopping_and_food", "一起去", [("看看", 8700), ("吃饭", 8100), ("逛逛", 7600)]),
    ("shopping_and_food", "想买", [("点东西", 8800), ("点水果", 8100), ("个新的", 7600)]),
    ("shopping_and_food", "买点", [("水果", 8800), ("东西", 8200), ("零食", 7600)]),
    ("shopping_and_food", "吃点", [("东西", 9000), ("水果", 8400), ("饭", 7700)]),
    ("shopping_and_food", "喝点", [("水", 9300), ("热水", 8600), ("茶", 8000)]),
    ("shopping_and_food", "这家", [("店不错", 8600), ("店怎么样", 8000), ("店挺好", 7500)]),
    ("shopping_and_food", "价格", [("合适", 8700), ("有点高", 8100), ("还可以", 7600)]),
    ("shopping_and_food", "味道", [("不错", 8900), ("很好", 8300), ("还可以", 7800)]),
    ("shopping_and_food", "再来", [("一点", 8800), ("一些", 8200), ("一份", 7700)]),
    ("shopping_and_food", "少放", [("一点", 9000), ("点盐", 8300), ("点糖", 7700)]),
    ("travel_and_arrival", "准备出发", [("了", 9000), ("的时候告诉我", 7900)]),
    ("travel_and_arrival", "快到", [("了", 9200), ("家的时候告诉我", 7700)]),
    ("travel_and_arrival", "到了", [("说一声", 8900), ("告诉我", 8300), ("再联系", 7700)]),
    ("travel_and_arrival", "出门", [("注意安全", 8800), ("记得带钥匙", 8200), ("前确认一下", 7600)]),
    ("travel_and_arrival", "回家", [("再说", 8700), ("休息一下", 8100), ("路上小心", 7500)]),
    ("travel_and_arrival", "路上有点", [("堵", 8600), ("慢", 7900)]),
    ("travel_and_arrival", "等车", [("的时候", 8500), ("有点久", 7800)]),
    ("travel_and_arrival", "今天晚上", [("有空吗", 8600), ("再联系", 8000), ("一起吃饭", 7400)]),
    ("travel_and_arrival", "明天下午", [("有空吗", 8600), ("再联系", 8000), ("方便吗", 7400)]),
    ("connecting_phrases", "如果", [("方便的话", 8900), ("可以的话", 8300), ("有时间", 7700)]),
    ("connecting_phrases", "所以", [("需要确认一下", 8700), ("先看看情况", 8100), ("暂时这样", 7500)]),
    ("connecting_phrases", "但是", [("还需要确认", 8600), ("不太确定", 8000), ("可以试试", 7400)]),
    ("connecting_phrases", "不过", [("可以试试", 8600), ("还要再看看", 8000), ("问题不大", 7500)]),
    ("connecting_phrases", "首先", [("确认一下", 8700), ("了解情况", 8100), ("整理材料", 7500)]),
    ("connecting_phrases", "然后", [("再确认一下", 8700), ("继续处理", 8100), ("再看看", 7500)]),
    ("connecting_phrases", "另外", [("还有一点", 8800), ("需要说明", 8200), ("请注意", 7600)]),
    ("connecting_phrases", "同时", [("需要注意", 8700), ("也要确认", 8100), ("继续跟进", 7500)]),
    ("connecting_phrases", "总体来说", [("还不错", 8800), ("比较合适", 8100), ("可以接受", 7500)]),
    ("connecting_phrases", "具体来说", [("需要确认", 8700), ("还要讨论", 8100), ("有几个问题", 7500)]),
]

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main() -> None:
    case_files = sorted((ROOT / "tests/data").glob("typing_*.tsv"))
    answers = {line.split("\t")[1] for path in case_files for line in path.read_text(encoding="utf-8-sig").splitlines()
               if line and not line.startswith("#")}
    rows, seen, excluded = [], set(), []
    for category, prefix, alternatives in GROUPS:
        assert re.fullmatch(r"[\u4e00-\u9fff]{2,8}", prefix), prefix
        for next_text, weight in alternatives:
            assert re.fullmatch(r"[\u4e00-\u9fff]{1,8}", next_text), next_text
            assert 100 <= weight <= 10000 and (prefix, next_text) not in seen
            seen.add((prefix, next_text))
            if prefix in answers or next_text in answers or prefix+next_text in answers:
                excluded.append({"prefix": prefix, "next_text": next_text}); continue
            rows.append((category, prefix, next_text, weight))
    rows.sort(key=lambda r: (r[0], r[1], -r[3], r[2]))
    output = BASE / "continuations_zh.tsv"
    lines, previous = [], None
    for category, prefix, next_text, weight in rows:
        if category != previous:
            lines.append(f"# category: {category}"); previous = category
        lines.append(f"{prefix}\t{next_text}\t{weight}")
    output.write_text("\n".join(lines)+"\n", encoding="utf-8", newline="\n")
    author = Path(__file__).resolve()
    meta = {
        "format_version": 1, "name": "LocalPinyinIME first-party next-text continuations",
        "columns": ["prefix", "next_text", "weight"], "encoding": "UTF-8", "entry_count": len(rows),
        "prefix_count": len({r[1] for r in rows}), "sha256": sha(output), "generated_at": "2026-10-12",
        "license": "MIT", "source_ids": ["first_party_continuations_001"], "runtime_dependencies": "none",
        "authoring_source": "first_party_authored", "review_status": "approved", "included_in_release": True,
        "authoring_script": "authoring/build_continuations.py", "authoring_script_sha256": sha(author),
        "category_counts": dict(sorted(Counter(r[0] for r in rows).items())),
        "prefix_length_range": [min(len(r[1]) for r in rows), max(len(r[1]) for r in rows)],
        "next_text_length_range": [min(len(r[2]) for r in rows), max(len(r[2]) for r in rows)],
        "weight_range": [min(r[3] for r in rows), max(r[3] for r in rows)],
        "weight_policy": "Explicit project-authored relative preference weights, not corpus frequencies or personal data",
        "authorship": "Individually authored ordinary next-text alternatives; no corpus, commercial IME or third-party lexicon import",
        "benchmark_sha256": {p.name: sha(p) for p in case_files}, "benchmark_answers_added": 0,
        "benchmark_overlaps_excluded": excluded, "validation": {"unique_pairs": True, "han_only": True,
            "prefix_two_to_eight_han": True, "next_one_to_eight_han": True, "weight_range_valid": True,
            "no_tabs_or_newlines_inside_fields": True, "full_frozen_answers_excluded": True},
    }
    output.with_suffix(".metadata.json").write_text(json.dumps(meta, ensure_ascii=False, indent=2)+"\n", encoding="utf-8", newline="\n")
    print(json.dumps({k: meta[k] for k in ("entry_count", "prefix_count", "sha256", "category_counts", "benchmark_overlaps_excluded")}, ensure_ascii=False))

if __name__ == "__main__":
    main()
