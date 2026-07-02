#!/usr/bin/env python3
"""Static installer checks that do not run Inno Setup or modify the system."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


def fail(message: str) -> None:
    raise AssertionError(message)


def section_lines(text: str, section_name: str) -> list[str]:
    in_section = False
    lines: list[str] = []
    for line in text.splitlines():
        stripped = line.strip()
        match = re.fullmatch(r"\[(.+)\]", stripped)
        if match:
            in_section = match.group(1).lower() == section_name.lower()
            continue
        if not in_section or not stripped or stripped.startswith(";"):
            continue
        lines.append(line)
    return lines


def inno_defines(text: str) -> dict[str, str]:
    defines: dict[str, str] = {}
    for match in re.finditer(
        r'(?im)^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+"([^"]*)"\s*$',
        text,
    ):
        defines[match.group(1)] = match.group(2)
    return defines


def resolve_preprocessor(value: str, defines: dict[str, str]) -> str:
    resolved = value
    for name, replacement in defines.items():
        resolved = resolved.replace(f"{{#{name}}}", replacement)
    return resolved


def split_inno_parameters(line: str) -> list[str]:
    parts: list[str] = []
    current: list[str] = []
    in_quote = False
    for ch in line:
        if ch == '"':
            in_quote = not in_quote
            current.append(ch)
            continue
        if ch == ";" and not in_quote:
            part = "".join(current).strip()
            if part:
                parts.append(part)
            current = []
            continue
        current.append(ch)
    part = "".join(current).strip()
    if part:
        parts.append(part)
    return parts


def parse_inno_directive(line: str, defines: dict[str, str]) -> dict[str, str]:
    fields: dict[str, str] = {}
    for part in split_inno_parameters(line):
        match = re.match(r"\s*([A-Za-z0-9_]+)\s*:\s*(.*)\s*$", part)
        if not match:
            continue
        value = match.group(2).strip()
        if len(value) >= 2 and value.startswith('"') and value.endswith('"'):
            value = value[1:-1]
        fields[match.group(1).lower()] = resolve_preprocessor(value, defines)
    return fields


def assert_start_menu_shortcuts(text: str) -> None:
    icon_lines = section_lines(text, "Icons")
    if not icon_lines:
        fail("missing [Icons] entries")

    defines = inno_defines(text)
    settings_entries: list[dict[str, str]] = []
    uninstall_entries: list[dict[str, str]] = []
    forbidden = re.compile(
        r"(^|\\)(build|dist)(\\|$)|LocalPinyinIME\.sln|user_lexicon\.tsv|"
        r"user_learning\.sqlite|\.pdb|test_",
        re.IGNORECASE,
    )
    hardcoded_localpinyin_path = re.compile(r"[A-Z]:\\.*LocalPinyinIME", re.IGNORECASE)

    for line in icon_lines:
        fields = parse_inno_directive(line, defines)
        values = [
            fields.get("name", ""),
            fields.get("filename", ""),
            fields.get("workingdir", ""),
        ]
        for value in values:
            if forbidden.search(value):
                fail(f"installer icon section references forbidden content: {line}")
            if hardcoded_localpinyin_path.search(value):
                fail(f"installer icon section uses a hard-coded LocalPinyinIME path: {line}")

        if (
            fields.get("name", "").lower() == r"{group}\localpinyinime 设置".lower()
            and fields.get("filename", "").lower()
            == r"{app}\LocalPinyinSettings.exe".lower()
        ):
            settings_entries.append(fields)
        if (
            fields.get("name", "").lower() == r"{group}\卸载 LocalPinyinIME".lower()
            and fields.get("filename", "").lower() == r"{uninstallexe}".lower()
        ):
            uninstall_entries.append(fields)

    if not settings_entries:
        fail(
            "settings shortcut must use Name {group}\\LocalPinyinIME 设置 "
            "and Filename {app}\\LocalPinyinSettings.exe"
        )
    if not any(entry.get("workingdir", "").lower() == r"{app}".lower() for entry in settings_entries):
        fail("settings shortcut must set WorkingDir to {app}")
    if not uninstall_entries:
        fail("uninstall shortcut must use Name {group}\\卸载 LocalPinyinIME and Filename {uninstallexe}")

    icons_section = "\n".join(icon_lines)
    if re.search(r"\{commondesktop\}|\{userdesktop\}|Desktop", icons_section, re.IGNORECASE):
        fail("installer must not create a desktop shortcut")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inno", type=Path, required=True)
    args = parser.parse_args()

    text = args.inno.read_text(encoding="utf-8")
    if "[Icons]" not in text:
        fail("missing [Icons] section")
    assert_start_menu_shortcuts(text)
    print("installer static shortcut checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
