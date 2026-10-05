import subprocess
import unittest
from unittest.mock import patch

from build_toolchain import native_frame_guard, normalize_windows_header


class NativeFrameGuardTests(unittest.TestCase):
    def flags(self, version):
        with patch('build_toolchain.subprocess.run', return_value=
                   subprocess.CompletedProcess([], 0, stdout=version)) as run:
            flags = native_frame_guard('compiler path with spaces')
            run.assert_called_once_with(['compiler path with spaces', '--version'],
                                        check=True, capture_output=True, text=True)
            return flags

    def test_llvm_mingw_preserves_existing_guard(self):
        self.assertEqual(self.flags('clang version 21.1.2'),
                         ['-Wframe-larger-than=16384', '-Werror=frame-larger-than'])

    def test_gcc_uses_parameterized_error_option(self):
        self.assertEqual(self.flags('x86_64-w64-mingw32-g++ (GCC) 15.2.0'),
                         ['-Werror=frame-larger-than=16384'])

    def test_unknown_compiler_refused(self):
        with self.assertRaises(ValueError):
            self.flags('unrecognized compiler')

    def test_failed_version_query_refused(self):
        with patch('build_toolchain.subprocess.run', side_effect=
                   subprocess.CalledProcessError(1, ['compiler', '--version'])):
            with self.assertRaises(subprocess.CalledProcessError):
                native_frame_guard('compiler')


class WindowsHeaderTests(unittest.TestCase):
    def test_platform_header_in_generated_copy(self):
        original = b'#include <Windows.h>\r\n#include <dxgi.h>\r\n'
        self.assertEqual(normalize_windows_header(original),
                         b'#include <windows.h>\r\n#include <dxgi.h>\r\n')
        self.assertIn(b'<Windows.h>', original)

    def test_normalization_is_idempotent(self):
        original = b'#include <windows.h>\n'
        self.assertEqual(normalize_windows_header(original), original)
