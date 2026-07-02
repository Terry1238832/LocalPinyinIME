#!/usr/bin/env python3

from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path


def run_tool(script: Path, lexicon: Path, *args: str, expect: int = 0) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        [sys.executable, str(script), "--path", str(lexicon), *args],
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if completed.returncode != expect:
        raise AssertionError(
            f"unexpected exit code {completed.returncode}, expected {expect}\n"
            f"stdout:\n{completed.stdout}\n"
            f"stderr:\n{completed.stderr}"
        )
    return completed


def require_contains(text: str, needle: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing output fragment: {needle!r}\n{text}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--script", required=True, type=Path)
    args = parser.parse_args()
    script = args.script

    private_word = "\u5bc6\u897f\u6c99\u52a0\u5927\u5b66"

    with tempfile.TemporaryDirectory(prefix="LocalPinyinIME-lexicon-test-") as temp:
        lexicon = Path(temp) / "user_lexicon.tsv"

        validate = run_tool(script, lexicon, "validate")
        require_contains(validate.stdout, "valid_entries: 0")
        if not lexicon.exists():
            raise AssertionError("validate did not create the missing lexicon")

        run_tool(script, lexicon, "add", "--pinyin", "UTM", "--word", private_word, "--frequency", "5000")
        run_tool(script, lexicon, "add", "--pinyin", "utm", "--word", private_word, "--frequency", "6000")
        run_tool(script, lexicon, "add", "--pinyin", "csc", "--word", "CSC108", "--frequency", "4000")

        listing = run_tool(script, lexicon, "list").stdout.splitlines()
        expected = [
            "csc\tCSC108\t4000",
            f"utm\t{private_word}\t6000",
        ]
        if listing != expected:
            raise AssertionError(f"list output was not stable\nexpected={expected!r}\nactual={listing!r}")

        report = run_tool(script, lexicon, "report").stdout
        require_contains(report, "valid_entries: 2")
        require_contains(report, "invalid_rows: 0")

        run_tool(script, lexicon, "add", "--pinyin", "csc108", "--word", "CSC108", "--frequency", "5000", expect=2)
        run_tool(script, lexicon, "add", "--pinyin", "", "--word", "\u7a7a", "--frequency", "1", expect=2)
        run_tool(script, lexicon, "add", "--pinyin", "bad", "--word", "bad\tword", "--frequency", "1", expect=2)
        run_tool(script, lexicon, "add", "--pinyin", "bad", "--word", "bad", "--frequency", "-1", expect=2)

        with lexicon.open("a", encoding="utf-8", newline="\n") as handle:
            handle.write("bad\trow\twith\textra\n")
        invalid = run_tool(script, lexicon, "validate", expect=1).stdout
        require_contains(invalid, "invalid_rows: 1")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
