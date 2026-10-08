"""Independent processes exercise provider-store locks and interrupted writes.

Run with the just-built provider_store_test executable; all data is synthetic.
"""
from pathlib import Path
import os
import subprocess
import struct
import sys
import tempfile
import unittest
import zlib

EXECUTABLE = os.environ.get("MCD2_PROVIDER_STORE_TEST", "")
if __name__ == "__main__" and len(sys.argv) > 1 and not sys.argv[1].startswith("-"):
    EXECUTABLE = str(Path(sys.argv.pop(1)).resolve())
OK, MISSING, INVALID, IO_ERROR, BUSY, CONFLICT = range(6)


@unittest.skipUnless(EXECUTABLE, "Run with the freshly built provider_store_test executable")
class StoreProcessTests(unittest.TestCase):
    def setUp(self):
        self.fixture = tempfile.TemporaryDirectory(prefix="mcd2-provider-")
        self.addCleanup(self.fixture.cleanup)
        self.root = Path(self.fixture.name) / "synthetic-世界"
        self.root.mkdir()

    def call(self, *args):
        result = subprocess.run([EXECUTABLE, str(self.root), *map(str, args)],
                                check=True, capture_output=True, text=True, timeout=20)
        return tuple(map(int, result.stdout.split()))

    def holding(self, command):
        child = subprocess.Popen([EXECUTABLE, str(self.root), command], stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        def cleanup():
            if child.poll() is None:
                child.kill()
            child.communicate(timeout=10)
        self.addCleanup(cleanup)
        # Timeout-aware read via a small reader future; avoid hanging a CI job.
        import concurrent.futures
        with concurrent.futures.ThreadPoolExecutor() as pool:
            ready = pool.submit(child.stdout.readline)
            try:
                self.assertEqual(ready.result(timeout=10), "ready\n")
            except BaseException:
                child.kill()
                raise
        return child

    def test_fixture_and_corruption_suite(self):
        subprocess.run([EXECUTABLE, str(self.root)], check=True, timeout=20)

    def test_wire_matches_independent_implementation(self):
        self.assertEqual(self.call("publish", 7, 0, 0)[0], OK)
        fields = [5, 7, 0] + [1, 6700, 7700] * 3
        fields += [1, 0, 0, 0, 2, 0, 1, 0, 1000, 203, 203, 0, 0, 0]
        expected = bytearray(struct.pack("<8s30I", b"MCD2GI5\0", 1, 128, 0, 0, *fields))
        struct.pack_into("<I", expected, 16, zlib.crc32(expected))
        self.assertEqual((self.root / "intent-v5.bin").read_bytes(), expected)

    def test_process_lock_and_abandoned_pending(self):
        initial = self.call("publish", 1, 0, 0)
        self.assertEqual(initial[0], OK)
        expected = initial[1:]
        held = self.holding("hold")
        self.assertEqual(self.call("publish", 2, *expected)[0], BUSY)
        held.kill()
        held.communicate(timeout=10)
        # OS lock release does not require deleting an orphaned lock sentinel.
        self.assertEqual(self.call("publish", 2, *expected)[0], OK)
        before = self.call("read")
        prepared = self.holding("prepare")
        self.assertTrue((self.root / "intent-v5.pending").is_file())
        self.assertEqual(self.call("read"), before)
        prepared.kill()
        prepared.communicate(timeout=10)
        self.assertEqual(self.call("read"), before)
        self.assertEqual(self.call("publish", 3, *before[1:])[0], OK)
        self.assertFalse((self.root / "intent-v5.pending").exists())

    def test_competing_writers_and_readers(self):
        import concurrent.futures
        original = self.call("publish", 1, 0, 0)
        # Two processes with the same old stamp cannot both commit.
        with concurrent.futures.ThreadPoolExecutor() as pool:
            outcomes = list(pool.map(lambda _: self.call("publish", 2, *original[1:]), range(2)))
        self.assertEqual(sum(result[0] == OK for result in outcomes), 1)
        self.assertTrue(all(result[0] in (OK, BUSY, CONFLICT) for result in outcomes))
        current = self.call("read")
        for revision in range(3, 18):
            with concurrent.futures.ThreadPoolExecutor() as pool:
                writer = pool.submit(self.call, "publish", revision, *current[1:])
                readers = [pool.submit(self.call, "read") for _ in range(4)]
                result = writer.result()
                self.assertEqual(result[0], OK)
                allowed = {current, result}
                for reader in readers:
                    # Readers see a complete old or complete new record only.
                    self.assertIn(reader.result(), allowed)
                current = result

    def test_legacy_files_untouched(self):
        legacy = {name: b"synthetic legacy " + bytes([n]) for n, name in enumerate((
            "MCD2GraphicsSettings.sav", "MCD2GraphicsFGSettings.sav", "MCD2GraphicsDisplaySettings.sav"))}
        for name, data in legacy.items():
            (self.root / name).write_bytes(data)
        self.assertEqual(self.call("publish", 31, 0, 0)[0], OK)
        for name, data in legacy.items():
            self.assertEqual((self.root / name).read_bytes(), data)


if __name__ == "__main__":
    unittest.main()
