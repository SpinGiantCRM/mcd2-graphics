"""Baseline provenance checks on disposable Git fixtures, never a game install."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    "provider_baseline", Path(__file__).resolve().parents[1] / "tools/provider_baseline.py")
baseline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(baseline)


class ProviderBaselineTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repo = Path(self.temp.name)
        self.git("init", "-q")
        self.git("config", "core.autocrlf", "false")
        self.write("src/native/example.hpp", b"first\nsecond\n")
        self.write("manifest.json", json.dumps({"version": "fixture", "files": {
            "Dungeons/Binaries/Win64/owned.ini": baseline.sha(b"payload")}}).encode())
        self.write("dependencies.lock.json", b'{"game":{"exeSHA256":"fixture"}}\n')
        self.commit()
        self.base = self.git("rev-parse", "HEAD").decode().strip()
        self.addCleanup(patch.stopall)
        patch.object(baseline, "RELEASE_SOURCE", self.base).start()
        self.snapshot = baseline.record(self.repo)

    def git(self, *args):
        return subprocess.check_output(["git", "-C", str(self.repo), *args], stderr=subprocess.PIPE)

    def write(self, name, data):
        path = self.repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    def commit(self):
        self.git("add", ".")
        self.git("-c", "user.name=Baseline Fixture", "-c", "user.email=fixture@example.invalid",
                 "commit", "-qm", "fixture")

    def test_canonical_git_bytes_ignore_checkout_line_endings(self):
        self.write("src/native/example.hpp", b"first\r\nsecond\r\n")
        self.assertEqual(self.snapshot, baseline.record(self.repo))
        report = baseline.verify(self.repo, self.snapshot, "HEAD")
        self.assertFalse(report["changed"])
        self.assertFalse(report["newRuntimeQualification"])

    def test_changed_added_removed_sources_are_reported(self):
        self.write("src/native/example.hpp", b"changed\n")
        self.write("src/latency/new.hpp", b"new\n")
        (self.repo / "manifest.json").unlink()
        self.commit()
        result = baseline.verify(self.repo, self.snapshot, "HEAD")
        self.assertEqual(result["changed"], ["src/native/example.hpp"])
        self.assertEqual(result["added"], ["src/latency/new.hpp"])
        self.assertEqual(result["removed"], ["manifest.json"])

    def test_changed_pins_or_incomplete_file_set_cannot_redefine_baseline(self):
        for field in ("implementationCommit", "ownedPayload", "game", "sourceFiles"):
            with self.subTest(field=field):
                invalid = copy.deepcopy(self.snapshot)
                invalid[field] = "changed" if field == "implementationCommit" else {}
                with self.assertRaises(ValueError):
                    baseline.verify(self.repo, invalid, "HEAD")

    def test_documentation_does_not_look_like_runtime_change(self):
        self.write("docs/new-plan.md", b"documentation only\n")
        self.commit()
        report = baseline.verify(self.repo, self.snapshot, "HEAD")
        self.assertEqual([report[k] for k in ("added", "removed", "changed")], [[], [], []])

    def test_payload_mismatch_reports_only_owned_relative_name(self):
        root = self.repo / "payload"
        root.mkdir()
        owned = self.snapshot["ownedPayload"]
        self.assertEqual(baseline.verify_payload(root, owned), list(owned))
        name = next(iter(owned))
        self.write("payload/" + name, b"payload")
        self.write("payload/PlayerSave.sav", b"unrelated private bytes")
        self.assertEqual(baseline.verify_payload(root, owned), [])
        self.write("payload/" + name, b"changed")
        self.assertEqual(baseline.verify_payload(root, owned), [name])

    def test_payload_paths_cannot_escape_selected_root(self):
        root = self.repo / "payload"
        root.mkdir()
        for name in ("../save.sav", "/absolute", "C:\\save.sav", "..\\save.sav"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                baseline.verify_payload(root, {name: "hash"})

    def test_symlink_escape_is_rejected_where_available(self):
        root = self.repo / "payload"
        root.mkdir()
        self.write("outside.ini", b"outside")
        try:
            (root / "owned.ini").symlink_to(self.repo / "outside.ini")
        except OSError:
            self.skipTest("Host does not permit unprivileged symlinks")
        with self.assertRaises(ValueError):
            baseline.verify_payload(root, {"owned.ini": baseline.sha(b"outside")})


if __name__ == "__main__":
    unittest.main()
