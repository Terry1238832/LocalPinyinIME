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
            fields.get("iconfilename", ""),
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
    if not any(entry.get("iconfilename", "").lower() == r"{app}\LocalPinyinSettings.exe".lower() for entry in settings_entries):
        fail("settings shortcut must set IconFilename to {app}\\LocalPinyinSettings.exe")
    if not uninstall_entries:
        fail("uninstall shortcut must use Name {group}\\卸载 LocalPinyinIME and Filename {uninstallexe}")
    if not any(entry.get("iconfilename", "").lower() == r"{app}\LocalPinyinSettings.exe".lower() for entry in uninstall_entries):
        fail("uninstall shortcut must set IconFilename to {app}\\LocalPinyinSettings.exe")

    icons_section = "\n".join(icon_lines)
    if re.search(r"\{commondesktop\}|\{userdesktop\}|Desktop", icons_section, re.IGNORECASE):
        fail("installer must not create a desktop shortcut")


def extract_pascal_block(text: str, name: str) -> str:
    pattern = re.compile(
        rf"(?:procedure|function)\s+{re.escape(name)}\b.*?(?=\n(?:procedure|function)\s+\w+\b|\n\[[A-Za-z]+\]|\Z)",
        re.IGNORECASE | re.DOTALL,
    )
    match = pattern.search(text)
    if not match:
        fail(f"missing Inno code block: {name}")
    return match.group(0)


def assert_setup_flow(text: str) -> None:
    required = [
        "PrivilegesRequired=admin",
        "SetupIconFile=..\\assets\\branding\\icon\\LocalPinyinIME.ico",
        "UninstallDisplayIcon={app}\\LocalPinyinSettings.exe",
        "DisableDirPage=yes",
        "DirExistsWarning=no",
        "UsePreviousAppDir=no",
        "function SetupDiagnosticLogPath(): String",
        "setup-diagnostics.log",
        "function RegistrationStatusLogPath(): String",
        r"LocalPinyinIME\logs\status.log",
        "ParamsWithDiagnostics(Params)",
        "--diagnostic-log",
        "LoadStringFromFile(SetupDiagnosticLogPath(), Contents)",
        "Exec(SetupToolPath(), ExecParams, SetupWorkingDir()",
        "VerifySystemRegistrationAfterRegister(NewDll, RegisterCode, PreviousDll)",
        "--verify --expected-dll",
        "verify target DLL after register-system",
        "--require-current-user-enabled",
        "RunRegisterAndVerify(NewDll, CurrentDll)",
        "RunEnableAndVerify(NewDll)",
        "preserving previous registration until new target verifies",
    ]
    lower_text = text.lower()
    for item in required:
        if item.lower() not in lower_text:
            fail(f"missing installer setup-flow text: {item}")

    if re.search(r"cmd\.exe", text, re.IGNORECASE):
        fail("installer must not invoke cmd.exe")
    if re.search(r"(?im)^\s*\[Run\]", text):
        fail("installer should use [Code] setup flow instead of [Run]")

    run_register = extract_pascal_block(text, "RunRegisterAndVerify").lower()
    if "registerparams := '--register-system --dll '" not in run_register:
        fail("RunRegisterAndVerify must construct --register-system parameters")
    if "runsetuptool(registerparams, 'register system', registercode)" not in run_register:
        fail("RunRegisterAndVerify must run register-system with its own exit code")
    if "verifysystemregistrationafterregister(newdll, registercode, previousdll)" not in run_register:
        fail("RunRegisterAndVerify must verify target DLL after register-system")
    if not (
        run_register.find("registerparams := '--register-system --dll '")
        < run_register.find("runsetuptool(registerparams, 'register system', registercode)")
        < run_register.find("verifysystemregistrationafterregister(newdll, registercode, previousdll)")
    ):
        fail("RunRegisterAndVerify sequence is out of order")

    target_verify = extract_pascal_block(text, "VerifySystemRegistrationAfterRegister").lower()
    if "--verify --expected-dll" not in target_verify:
        fail("target DLL verification must use --verify --expected-dll")
    if "'verify target dll after register-system'" not in target_verify:
        fail("target DLL verification step name is missing")
    if not (
        target_verify.find("registereddllmatches(newdll)")
        < target_verify.find("targetverifyparams := '--verify --expected-dll '")
        < target_verify.find("runsetuptool(targetverifyparams, 'verify target dll after register-system'")
    ):
        fail("target DLL verification sequence is out of order")

    run_enable = extract_pascal_block(text, "RunEnableAndVerify").lower()
    if "'--enable-current-user', 'enable current user'" not in run_enable:
        fail("RunEnableAndVerify must enable current user explicitly")
    if "'verify after enable-current-user'" not in run_enable:
        fail("RunEnableAndVerify must run a final verify")
    if "--require-current-user-enabled" not in run_enable:
        fail("final verification must require current-user enabled state")
    if not (
        run_enable.find("'--enable-current-user', 'enable current user'")
        < run_enable.find("finalverifyparams := '--verify --expected-dll '")
        < run_enable.find("'verify after enable-current-user'")
    ):
        fail("enable-current-user -> final verify sequence is out of order")

    cur_step = extract_pascal_block(text, "CurStepChanged").lower()
    if "--unregister-system" in cur_step:
        fail("CurStepChanged must not unregister the previous DLL before the new DLL verifies")
    run_register_index = cur_step.find("runregisterandverify(newdll, currentdll)")
    run_enable_index = cur_step.find("runenableandverify(newdll)")
    if run_register_index < 0 or run_enable_index < 0:
        fail("CurStepChanged must call register and enable phases")
    if not run_register_index < run_enable_index:
        fail("CurStepChanged must run register before enable")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inno", type=Path, required=True)
    args = parser.parse_args()

    text = args.inno.read_text(encoding="utf-8")
    if "[Icons]" not in text:
        fail("missing [Icons] section")
    assert_start_menu_shortcuts(text)
    assert_setup_flow(text)
    print("installer static shortcut checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
