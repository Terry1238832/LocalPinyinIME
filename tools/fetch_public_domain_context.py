"""Authoring-only download of independently attributed public-domain prose.

Training works are deliberately distinct from the frozen article test set.
Fetch once, inspect the extracted text, then build_context_model.py is offline.
"""
from __future__ import annotations
import argparse
from concurrent.futures import ThreadPoolExecutor
from html.parser import HTMLParser
import hashlib
import json
from pathlib import Path
import re
from urllib.parse import quote
from urllib.request import Request, urlopen

WORKS = ["秋夜 (鲁迅)", "好的故事", "风筝 (鲁迅)", "聪明人和傻子和奴才",
         "从百草园到三味书屋", "藤野先生", "阿长与山海经", "五猖会", "社戏",
         "兔和猫", "鸭的喜剧", "孔乙己", "药", "明天", "头发的故事", "一件小事"]
# 一件小事 is evaluated, so it must never be fetched as training.
WORKS = [work for work in WORKS if work not in {"一件小事", "背影", "故乡"}]


class ArticleParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.depth = 0
        self.article_depth = 0
        self.table_depth = 0
        self.p_depth = 0
        self.buffer = []
        self.paragraphs = []

    def handle_starttag(self, tag, attrs):
        if tag == "div":
            self.depth += 1
            if "mw-parser-output" in dict(attrs).get("class", "").split():
                self.article_depth = self.depth
        if self.article_depth:
            if tag == "table": self.table_depth += 1
            if tag == "p" and not self.table_depth:
                self.p_depth += 1
                self.buffer = []

    def handle_endtag(self, tag):
        if tag == "p" and self.p_depth:
            self.p_depth -= 1
            value = "".join(self.buffer).replace("\u200b", "").strip()
            # Editor notices/transcription metadata are not prose.
            if len(re.findall(r"[\u3400-\u9fff]", value)) >= 6 and not any(
                    text in value for text in ("公有领域", "本作品收录", "版权期限", "本站的全部文字")):
                self.paragraphs.append(value)
        if tag == "table" and self.table_depth: self.table_depth -= 1
        if tag == "div":
            if self.article_depth == self.depth: self.article_depth = 0
            self.depth -= 1

    def handle_data(self, value):
        if self.article_depth and self.p_depth and not self.table_depth:
            self.buffer.append(value)


def fetch(work, destination):
    url = "https://zh.wikisource.org/zh-hans/" + quote(work)
    with urlopen(Request(url, headers={"User-Agent": "LocalPinyinIME-authoring/0.5 (public-domain corpus)"}), timeout=45) as response:
        html = response.read().decode("utf-8")
    if "PD-old" not in html and "公有领域" not in html:
        raise ValueError(f"Public-domain notice missing: {work}")
    parser = ArticleParser()
    parser.feed(html)
    if sum(len(x) for x in parser.paragraphs) < 300:
        raise ValueError(f"Not enough extracted prose: {work}")
    filename = "luxun_" + re.sub(r"[^\w]", "_", work) + ".txt"
    text = "# Author: 鲁迅 (1881–1936). Public-domain original prose.\n# Source: " + url + "\n" + "\n".join(parser.paragraphs) + "\n"
    path = destination / filename
    path.write_text(text, encoding="utf-8")
    return {"file": filename, "title": work, "author": "鲁迅", "author_died": 1936,
            "source": url, "rights": "public-domain original prose; source page PD notice verified",
            "retrieved": "2026-10-12", "transformation": "Simplified display; article paragraphs only; punctuation retained",
            "training_excluded_from_holdout": True, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--destination", type=Path, required=True)
    a = p.parse_args()
    a.destination.mkdir(parents=True, exist_ok=True)
    sources = [{"file": path.name, "rights": "MIT; original authored prose", "author": "LocalPinyinIME contributors"}
               for path in sorted(a.destination.glob("daily_*.txt"))]
    failures = []
    with ThreadPoolExecutor(max_workers=3) as pool:
        futures = [(work, pool.submit(fetch, work, a.destination)) for work in WORKS]
        for work, future in futures:
            try:
                sources.append(future.result())
            except Exception as error:
                failures.append({"title": work, "error": str(error)})
    manifest = {"format_version": 1, "heldout_works_excluded": ["背影", "故乡", "一件小事"],
                "runtime_artifact": "../context_zh.tsv; only 1-4 character counts are packaged", "sources": sources,
                "fetch_failures": failures}
    (a.destination / "sources.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(json.dumps({"fetched": sum("source" in item for item in sources), "failures": failures}, ensure_ascii=True))


if __name__ == "__main__":
    main()
