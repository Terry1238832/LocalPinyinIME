"""Exercise the Electron/native boundary in separate, isolated processes."""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
import unittest


BRIDGE: Path
TEMP_ROOT: Path | None = None


class SettingsBridgeContract(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="settings-bridge-", dir=TEMP_ROOT)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.appdata = self.root / "appdata"
        self.appdata.mkdir()
        self.data = self.appdata / "LocalPinyinIME"
        self.lexicon = self.data / "user_lexicon.tsv"
        self.env = dict(os.environ)
        self.env["LOCALPINYINIME_LOCALAPPDATA"] = str(self.appdata)
        self.env["LOCALPINYINIME_DIAGNOSTICS"] = "1"
        self.env.pop("LOCALPINYINIME_USER_LEXICON_PATH", None)

    def call(self, *arguments: str, success: bool = True, env: dict[str, str] | None = None) -> dict:
        result = subprocess.run(
            [str(BRIDGE), *arguments], env=env or self.env, capture_output=True,
            encoding="utf-8", errors="strict", timeout=30, check=False,
        )
        self.assertEqual(result.stderr, "", "The private settings API must not log request values")
        # Any extra banner, diagnostics, or multiple responses breaks this parse.
        value = json.loads(result.stdout)
        self.assertIsInstance(value, dict)
        if success:
            self.assertEqual(result.returncode, 0, value)
            self.assertNotIn("error", value)
        else:
            self.assertNotEqual(result.returncode, 0)
            self.assertIsInstance(value.get("error"), str)
            self.assertTrue(value["error"])
        return value

    def snapshot(self) -> dict[str, tuple]:
        result = {}
        for path in self.root.rglob("*"):
            relative = path.relative_to(self.root).as_posix()
            if path.is_dir():
                result[relative] = ("directory",)
            else:
                result[relative] = ("file", path.stat().st_mtime_ns,
                                    hashlib.sha256(path.read_bytes()).hexdigest())
        return result

    def test_first_open_and_previews_do_not_write_user_files(self) -> None:
        before = self.snapshot()
        state = self.call("state")
        self.assertRegex(state["version"], r"^\d+\.\d+\.\d+")
        self.assertIn(state["settings"]["theme"], {"system", "light", "dark"})
        self.assertIn(state["settings"]["textSize"], range(1, 8))
        self.assertIsInstance(state["settings"]["shiftToggleEnabled"], bool)
        self.assertIs(state["settings"]["predictionEnabled"], False)
        self.assertGreater(state["dictionary"]["total"], 1000)
        self.assertTrue(all(state["dictionary"][key] > 0 for key in ("core", "local", "standard")))
        self.assertEqual(state["learning"], {"count": 0, "capacity": 2048})
        self.assertFalse(state["runtime"]["available"])
        self.assertEqual(Path(state["dataDirectory"]), self.data)
        self.assertEqual(state["userLexiconCount"], 0)
        self.assertEqual(self.call("user-list"), {"entries": []})
        self.assertEqual(self.call("lookup", "suibian")["candidates"][0], "随便")
        self.assertEqual(self.call("lookup", ""), {"candidates": [], "pinyin": ""})
        self.assertEqual(self.call("lookup", "suibian")["pinyin"], "sui'bian")
        self.assertIn("去", self.call("predict", "我想")["candidates"])
        self.assertEqual(self.call("predict", ""), {"candidates": []})
        matches = self.call("dictionary-search", "随便", "5")
        self.assertTrue(any(entry["word"] == "随便" for entry in matches["entries"]))
        self.assertGreaterEqual(matches["total"], len(matches["entries"]))
        self.assertEqual(self.snapshot(), before)

    def test_settings_round_trip_preserves_existing_candidate_option(self) -> None:
        self.data.mkdir()
        settings = self.data / "settings.json"
        settings.write_text(json.dumps({
            "candidateWindow": {"theme": "light", "textSize": "standard", "showKeyHints": False},
            "inputMode": {"shiftToggleEnabled": False},
        }), encoding="utf-8")
        for theme, size, shift in (("dark", "7", "true"), ("system", "1", "false"), ("light", "4", "true")):
            expected = {"theme": theme, "textSize": int(size), "candidateStyle": "flow",
                        "shiftToggleEnabled": shift == "true", "predictionEnabled": False, "typoCorrection": True, "mixedEnglish": True, "fuzzyPinyin": [], "doublePinyin": "none", "usageDomains": [], "digitPeriod": True, "codePunctuation": True}
            self.assertEqual(self.call("save-settings", theme, size, shift), {"settings": expected})
            self.assertEqual(self.call("state")["settings"], expected)
            self.assertFalse(json.loads(settings.read_text(encoding="utf-8"))["candidateWindow"]["showKeyHints"])
        self.assertFalse(self.lexicon.exists())

    def test_candidate_style_is_saved_independently_and_falls_back(self) -> None:
        # Settings from before 0.12 have no style: the default applies and
        # every other option survives the first save.
        self.data.mkdir()
        settings = self.data / "settings.json"
        settings.write_text(json.dumps({
            "candidateWindow": {"theme": "dark", "textSize": "level6", "showKeyHints": False},
            "inputMode": {"shiftToggleEnabled": False, "nextWordPrediction": True},
        }), encoding="utf-8")
        self.assertEqual(self.call("state")["settings"], {"theme": "dark", "textSize": 6, "candidateStyle": "flow",
                                                          "shiftToggleEnabled": False, "predictionEnabled": True, "typoCorrection": True, "mixedEnglish": True, "fuzzyPinyin": [], "doublePinyin": "none", "usageDomains": [], "digitPeriod": True, "codePunctuation": True})
        for style in ("rail", "type", "glass", "flow"):
            saved = self.call("save-settings", "dark", "6", "false", "true", style)["settings"]
            self.assertEqual(saved, {"theme": "dark", "textSize": 6, "candidateStyle": style,
                                     "shiftToggleEnabled": False, "predictionEnabled": True, "typoCorrection": True, "mixedEnglish": True, "fuzzyPinyin": [], "doublePinyin": "none", "usageDomains": [], "digitPeriod": True, "codePunctuation": True})
            self.assertEqual(self.call("state")["settings"], saved)
            stored = json.loads(settings.read_text(encoding="utf-8"))["candidateWindow"]
            self.assertEqual(stored["style"], style)
            self.assertFalse(stored["showKeyHints"])
        # Changing the theme or size without a style keeps the saved style.
        self.call("save-settings", "light", "2", "true", "true", "type")
        self.assertEqual(self.call("save-settings", "system", "3", "true")["settings"]["candidateStyle"], "type")
        # Unknown styles are rejected on save, and fall back when read.
        self.call("save-settings", "light", "4", "true", "true", "aurora", success=False)
        self.assertEqual(self.call("state")["settings"]["candidateStyle"], "type")
        stored = json.loads(settings.read_text(encoding="utf-8"))
        stored["candidateWindow"]["style"] = "aurora"
        settings.write_text(json.dumps(stored), encoding="utf-8")
        self.assertEqual(self.call("state")["settings"]["candidateStyle"], "flow")
        self.assertEqual(self.call("state")["settings"]["textSize"], 3)

    def test_input_options_are_saved_validated_and_used_by_lookup(self) -> None:
        self.data.mkdir()
        settings = self.data / "settings.json"
        # Defaults: correction and mixed input on, no fuzzy pairs, full pinyin.
        state = self.call("state")["settings"]
        self.assertEqual((state["typoCorrection"], state["mixedEnglish"], state["fuzzyPinyin"], state["doublePinyin"]),
                         (True, True, [], "none"))
        self.assertEqual(self.call("lookup", "nihso")["candidates"][0], "你好")
        self.assertEqual(self.call("lookup", "ppt")["candidates"][0], "PPT")
        self.assertNotIn("中国", self.call("lookup", "zongguo")["candidates"])
        # Named options follow the positional ones and change only themselves.
        saved = self.call("save-settings", "dark", "5", "true", "false", "rail",
                          "fuzzyPinyin=z-zh,n-l,in-ing", "typoCorrection=false")["settings"]
        self.assertEqual(saved["fuzzyPinyin"], ["z-zh", "n-l", "in-ing"])
        self.assertEqual((saved["typoCorrection"], saved["mixedEnglish"], saved["candidateStyle"], saved["theme"]),
                         (False, True, "rail", "dark"))
        stored = json.loads(settings.read_text(encoding="utf-8"))["inputMode"]
        self.assertEqual(stored["fuzzyPinyin"], "z-zh,n-l,in-ing")
        self.assertIs(stored["typoCorrection"], False)
        # The preview types with the saved options.
        self.assertEqual(self.call("lookup", "zongguo")["candidates"][0], "中国")
        self.assertNotEqual(self.call("lookup", "nihso")["candidates"][0], "你好")
        # A save without named options keeps them.
        self.assertEqual(self.call("save-settings", "light", "4", "false")["settings"]["fuzzyPinyin"], ["z-zh", "n-l", "in-ing"])
        # Double pinyin: the preview converts key pairs and shows full pinyin.
        self.call("save-settings", "light", "4", "false", "false", "flow", "doublePinyin=xiaohe", "fuzzyPinyin=")
        lookup = self.call("lookup", "nihc")
        self.assertEqual((lookup["candidates"][0], lookup["pinyin"]), ("你好", "ni'hao"))
        self.call("save-settings", "light", "4", "false", "false", "flow", "doublePinyin=ziranma")
        self.assertEqual(self.call("lookup", "nihk")["candidates"][0], "你好")
        # Mixed input off: English terms are not recognized.
        self.call("save-settings", "light", "4", "false", "false", "flow", "doublePinyin=none", "mixedEnglish=false")
        self.assertNotIn("PPT", self.call("lookup", "ppt")["candidates"])
        before = settings.read_bytes()
        for bad in ("doublePinyin=qwerty", "fuzzyPinyin=z-zh,zz", "typoCorrection=yes", "unknownOption=1", "noequals"):
            self.call("save-settings", "light", "4", "false", "false", "flow", bad, success=False)
        self.assertEqual(settings.read_bytes(), before)
        # Unknown values in the file fall back to the defaults.
        stored = json.loads(settings.read_text(encoding="utf-8"))
        stored["inputMode"]["doublePinyin"] = "qwerty"
        stored["inputMode"]["fuzzyPinyin"] = "zz,n-l"
        settings.write_text(json.dumps(stored), encoding="utf-8")
        state = self.call("state")["settings"]
        self.assertEqual((state["doublePinyin"], state["fuzzyPinyin"]), ("none", ["n-l"]))

    def test_usage_scenes_are_saved_listed_and_used_by_lookup(self) -> None:
        self.data.mkdir()
        settings = self.data / "settings.json"
        self.assertEqual(self.call("state")["settings"]["usageDomains"], [])
        # Every pack is listed for the settings page.
        packs = self.call("domain-words")["domains"]
        self.assertEqual(list(packs), ["coding", "work", "math", "study", "daily", "games"])
        self.assertTrue(all(len(words) >= 50 for words in packs.values()))
        self.assertIn("其实", packs["daily"])
        self.assertIn("依赖注入", packs["coding"])
        # Without scenes the old variant leads; the daily scene lifts 好像是.
        self.assertEqual(self.call("lookup", "haoxiangshi")["candidates"][0], "好象是")
        saved = self.call("save-settings", "light", "4", "false", "false", "flow", "usageDomains=coding,daily")["settings"]
        self.assertEqual(saved["usageDomains"], ["coding", "daily"])
        self.assertEqual(json.loads(settings.read_text(encoding="utf-8"))["inputMode"]["usageDomains"], "coding,daily")
        self.assertEqual(self.call("lookup", "haoxiangshi")["candidates"][0], "好像是")
        self.assertEqual(self.call("lookup", "qishi", "所以整道题")["candidates"][0], "其实")
        self.assertEqual(self.call("lookup", "yilaizhuru")["candidates"][0], "依赖注入")
        # Ids must be known and in the canonical order.
        before = settings.read_bytes()
        for bad in ("usageDomains=daily,coding", "usageDomains=cooking", "usageDomains=coding,"):
            self.call("save-settings", "light", "4", "false", "false", "flow", bad, success=False)
        self.assertEqual(settings.read_bytes(), before)
        # An unknown id in the file is dropped; clearing restores the plain order.
        stored = json.loads(settings.read_text(encoding="utf-8"))
        stored["inputMode"]["usageDomains"] = "cooking,math"
        settings.write_text(json.dumps(stored), encoding="utf-8")
        self.assertEqual(self.call("state")["settings"]["usageDomains"], ["math"])
        self.call("save-settings", "light", "4", "false", "false", "flow", "usageDomains=")
        self.assertEqual(self.call("lookup", "haoxiangshi")["candidates"][0], "好象是")

    def test_typing_helpers_are_saved_and_validated(self) -> None:
        self.data.mkdir()
        settings = self.data / "settings.json"
        # Both helpers are on by default: 3. after a digit, ASCII punctuation in code editors.
        state = self.call("state")["settings"]
        self.assertEqual((state["digitPeriod"], state["codePunctuation"]), (True, True))
        saved = self.call("save-settings", "light", "4", "false", "false", "flow",
                          "digitPeriod=false", "codePunctuation=false")["settings"]
        self.assertEqual((saved["digitPeriod"], saved["codePunctuation"]), (False, False))
        stored = json.loads(settings.read_text(encoding="utf-8"))["inputMode"]
        self.assertEqual((stored["digitPeriod"], stored["codePunctuation"]), (False, False))
        before = settings.read_bytes()
        for bad in ("digitPeriod=yes", "codePunctuation=1"):
            self.call("save-settings", "light", "4", "false", "false", "flow", bad, success=False)
        self.assertEqual(settings.read_bytes(), before)
        self.call("save-settings", "light", "4", "false", "false", "flow", "digitPeriod=true")
        self.assertEqual(self.call("state")["settings"]["digitPeriod"], True)

    def test_context_preview_and_prediction_persistence(self) -> None:
        before = self.snapshot()
        predicted = self.call("predict", "我想")["candidates"]
        self.assertIn("去", predicted)
        self.assertGreater(len(predicted), 0)
        self.assertLessEqual(len(predicted), 9)
        self.assertEqual(len(predicted), len(set(predicted)))
        self.assertNotIn("我想去", predicted)
        self.assertEqual(self.call("predict", "我想。"), {"candidates": []})
        self.assertTrue(self.call("lookup", "qu", "我想")["candidates"])
        self.call("predict", "我" * 33, success=False)
        self.call("lookup", "qu", "我" * 33, success=False)
        self.call("lookup", "qu", "我\n想", success=False)
        self.call("save-settings", "light", "4", "false", "invalid", success=False)
        self.assertEqual(self.snapshot(), before)
        settings = self.call("save-settings", "dark", "5", "true", "true")["settings"]
        self.assertIs(settings["predictionEnabled"], True)
        self.assertEqual(self.call("state")["settings"], settings)
        # Legacy callers that only change theme/font/Shift preserve the option.
        self.assertIs(self.call("save-settings", "light", "4", "false")["settings"]["predictionEnabled"], True)
        # Settings saved by 0.7 (always predictionEnabled=true) do not opt in.
        legacy = self.data / "settings.json"
        legacy.write_text(json.dumps({"inputMode": {"predictionEnabled": True}}), encoding="utf-8")
        self.assertIs(self.call("state")["settings"]["predictionEnabled"], False)
        self.call("save-settings", "light", "4", "false", "false")
        stopped = self.snapshot()
        self.assertEqual(self.call("predict", "我想")["candidates"], predicted)
        self.call("lookup", "qu", "我想")
        self.assertEqual(self.snapshot(), stopped)

    def test_personal_crud_edit_rename_unicode_and_restart(self) -> None:
        first = {"pinyin": "qingshikejifazhanyouxiangongsi", "word": "青石科技发展有限公司", "frequency": 5000}
        response = self.call("user-upsert", first["pinyin"].upper(), first["word"], "5000")
        self.assertEqual(response, {"entries": [first]})
        self.assertEqual(self.call("lookup", first["pinyin"])["candidates"][0], first["word"])
        self.assertEqual(self.call("user-list"), response)
        special = '名字"引号\\路径🙂\x01'
        self.call("user-upsert", "teshuciyu", special, "1234")
        # Edit changes both key and text in one atomic replacement, preserving
        # the unrelated entry and eliminating the original rather than cloning.
        updated = self.call("user-upsert", "teshuxinci", special + "新", "6000", "teshuciyu", special)
        self.assertEqual(len(updated["entries"]), 2)
        self.assertIn(first, updated["entries"])
        self.assertIn({"pinyin": "teshuxinci", "word": special + "新", "frequency": 6000}, updated["entries"])
        self.assertNotIn("teshuciyu", {entry["pinyin"] for entry in updated["entries"]})
        self.assertEqual(self.call("state")["userLexiconCount"], 2)
        self.assertEqual(self.call("user-list"), updated)
        before = self.snapshot()
        self.call("lookup", first["pinyin"])
        self.call("state")
        self.call("user-list")
        self.call("dictionary-search", "suibian")
        self.assertEqual(self.snapshot(), before)
        removed = self.call("user-delete", "TESHUXINCI", special + "新")
        self.assertEqual(removed, {"entries": [first]})
        self.assertEqual(self.call("user-delete", first["pinyin"], first["word"]), {"entries": []})
        self.assertEqual(self.call("user-list"), {"entries": []})
        self.assertFalse(Path(str(self.lexicon) + ".tmp").exists())

    def test_invalid_edits_never_replace_valid_data(self) -> None:
        self.call("user-upsert", "baoliu", "保留", "5000")
        before = self.snapshot()
        bad_commands = [
            (), ("unknown",), ("state", "extra"), ("user-list", "extra"),
            ("save-settings", "wrong", "4", "true"),
            ("save-settings", "dark", "8", "true"),
            ("save-settings", "dark", "0", "false"),
            ("save-settings", "dark", "4", "1"),
            ("user-upsert", "ni hao", "你好", "5000"),
            ("user-upsert", "nihao", "你好\t坏词", "5000"),
            ("user-upsert", "nihao", "", "5000"),
            ("user-upsert", "a" * 257, "长", "5000"),
            ("user-upsert", "chang", "长" * 129, "5000"),
            ("user-upsert", "nihao", "你好", "-1"),
            ("user-upsert", "nihao", "你好", "1000001"),
            ("user-upsert", "nihao", "你好", "5000", "deleted", "不存在"),
            ("user-delete", "deleted", "不存在"),
            ("lookup", "a" * 257), ("lookup", "bad;echo"),
            ("dictionary-search", "长" * 129),
            ("dictionary-search", "词", "0"), ("dictionary-search", "词", "501"),
            ("clear-learning", "extra"),
        ]
        for arguments in bad_commands:
            with self.subTest(command=arguments[:1]):
                self.call(*arguments, success=False)
                self.assertEqual(self.snapshot(), before)

    def test_read_catalog_matches_limits_and_unknown_search(self) -> None:
        all_state = self.call("state")
        limited = self.call("dictionary-search", "", "3")
        self.assertEqual(len(limited["entries"]), 3)
        self.assertEqual(limited["total"], all_state["dictionary"]["total"])
        for entry in limited["entries"]:
            self.assertIn(entry["source"], {"core", "local", "standard"})
            self.assertIsInstance(entry["frequency"], int)
        self.assertEqual(self.call("dictionary-search", "SU I BIAN"), {"entries": [], "total": 0})
        self.assertEqual(self.call("dictionary-search", "SU I BIAN"), self.call("dictionary-search", "su i bian"))
        self.assertTrue(any(entry["word"] == "随便" for entry in self.call("dictionary-search", "SUIBIAN")["entries"]))

    def test_learning_reset_does_not_change_settings_or_personal_entries(self) -> None:
        self.call("save-settings", "dark", "5", "true")
        personal = self.call("user-upsert", "baoliu", "保留", "5000")
        learning = self.data / "user_learning.tsv"
        learning.write_text("nihao\t你好\t2\t1\nnihao\t你好\t4\t2\nsuibian\t随便\t1\t3\nbad row\n", encoding="utf-8")
        self.assertEqual(self.call("state")["learning"], {"count": 2, "capacity": 2048})
        before = self.snapshot()
        self.call("lookup", "suibian")
        self.call("state")
        self.assertEqual(self.snapshot(), before)
        self.assertEqual(self.call("clear-learning"), {"cleared": True})
        self.assertFalse(learning.exists())
        self.assertEqual(self.call("state")["learning"], {"count": 0, "capacity": 2048})
        self.assertEqual(self.call("user-list"), personal)
        self.assertEqual(self.call("state")["settings"], {"theme": "dark", "textSize": 5, "candidateStyle": "flow",
                                                          "shiftToggleEnabled": True, "predictionEnabled": False, "typoCorrection": True, "mixedEnglish": True, "fuzzyPinyin": [], "doublePinyin": "none", "usageDomains": [], "digitPeriod": True, "codePunctuation": True})
        self.assertEqual(self.call("clear-learning"), {"cleared": True})

    def test_valid_runtime_status_is_reported_and_not_rewritten(self) -> None:
        self.data.mkdir()
        runtime = self.data / "runtime_status.json"
        runtime.write_text(json.dumps({
            "schemaVersion": 1, "active": True, "mode": "English", "source": "contract-test",
            "processId": os.getpid(), "threadId": 1, "sequence": 42,
            "updatedAtUtcMs": int(time.time() * 1000),
        }), encoding="utf-8")
        before = self.snapshot()
        self.assertEqual(self.call("state")["runtime"], {"active": True, "mode": "English", "available": True})
        self.assertEqual(self.snapshot(), before)

    def test_override_lexicon_path_and_concurrent_updates_preserve_all_words(self) -> None:
        alternate = self.root / "custom" / "词库.tsv"
        env = dict(self.env, LOCALPINYINIME_USER_LEXICON_PATH=str(alternate))
        before = self.snapshot()
        self.assertEqual(self.call("user-list", env=env), {"entries": []})
        self.call("lookup", "suibian", env=env)
        self.assertEqual(self.snapshot(), before)
        fixtures = [("jiayi", "甲一"), ("jiayi", "甲二"), ("yibing", "乙丙"), ("bingding", "丙丁")]
        with ThreadPoolExecutor(max_workers=4) as pool:
            futures = [pool.submit(self.call, "user-upsert", key, word, "5000", env=env) for key, word in fixtures]
            for future in futures:
                future.result()
        self.assertEqual({(entry["pinyin"], entry["word"]) for entry in self.call("user-list", env=env)["entries"]}, set(fixtures))
        self.assertTrue(alternate.exists())
        self.assertFalse(self.lexicon.exists())

    def test_malformed_personal_file_is_preserved_on_failed_mutation(self) -> None:
        self.data.mkdir()
        self.lexicon.write_text("baoliu\t保留\t5000\nINVALID PRIVATE LINE\n", encoding="utf-8")
        before = self.snapshot()
        self.assertEqual(self.call("user-list")["entries"], [{"pinyin": "baoliu", "word": "保留", "frequency": 5000}])
        self.call("user-upsert", "xin", "新", "5000", success=False)
        self.call("user-delete", "baoliu", "保留", success=False)
        self.assertEqual(self.snapshot(), before)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--bridge", required=True, type=Path)
    parser.add_argument("--temp-root", type=Path)
    arguments, remaining = parser.parse_known_args()
    # CTest supplies the trusted target path. Do not open the executable merely
    # to canonicalize it: restricted Windows test processes may execute a
    # generated binary without permission to query its final path by handle.
    BRIDGE = arguments.bridge.absolute()
    TEMP_ROOT = arguments.temp_root
    if TEMP_ROOT:
        TEMP_ROOT.mkdir(parents=True, exist_ok=True)
    unittest.main(argv=[__file__, *remaining])
