import io
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import store_probe


def fixture():
    data = bytearray(1024)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 60, 128)
    data[128:132] = b'PE\0\0'
    struct.pack_into('<HHI', data, 132, 0x8664, 1, 0)
    struct.pack_into('<H', data, 148, 240)
    struct.pack_into('<H', data, 152, 0x20b)
    struct.pack_into('<I', data, 208, 0x2000)
    data[392:397] = b'.text'
    struct.pack_into('<IIII', data, 400, 32, 0x1000, 32, 512)
    data[516:519] = b'abc'
    return bytes(data)


class StoreProbeChecks(unittest.TestCase):
    def test_bounded_rva_conversion(self):
        data = fixture()
        with patch.dict(store_probe.BOUNDARIES, {'sample': (0x1004, b'abc'), 'outside': (0x5000, b'a')}, clear=True):
            result = store_probe.pe_metadata(io.BytesIO(data), len(data))
        self.assertEqual(result['onDiskSteamBoundaryMatches'], {'sample': True, 'outside': False})
        self.assertFalse(result['diskSignaturesEstablishRuntimeABI'])
        self.assertEqual(result['architecture'], 'AMD64')

    def test_corrupt_headers_declined(self):
        for data in (b'opaque', fixture()[:400], bytes(1024)):
            with self.assertRaises(ValueError):
                store_probe.pe_metadata(io.BytesIO(data), len(data))

    def test_section_outside_file_declined(self):
        data = bytearray(fixture())
        struct.pack_into('<I', data, 412, 5000)
        with self.assertRaises(ValueError):
            store_probe.pe_metadata(io.BytesIO(data), len(data))

    def test_report_never_enables_store_or_emits_path(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'Dungeons-WinGDK-Shipping.exe'
            path.write_bytes(fixture())
            before = path.read_bytes()
            result = store_probe.inspect(path, 'unverified')
            self.assertFalse(result['StoreSupportEnabled'])
            self.assertFalse(result['runtimeQualified'])
            self.assertFalse(result['exactVerifiedSteamBuild'])
            self.assertNotIn(folder, str(result))
            self.assertEqual(path.read_bytes(), before)

    def test_unreadable_file_safe_report(self):
        result = store_probe.inspect(Path('missing/Dungeons-WinGDK-Shipping.exe'), 'unverified')
        self.assertEqual(result['fileReadStatus'], 'unavailable')
        self.assertFalse(result['StoreSupportEnabled'])


if __name__ == '__main__':
    unittest.main()
