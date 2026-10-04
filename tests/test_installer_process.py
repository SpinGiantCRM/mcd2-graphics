"""Regression checks for Windows' full executable-name process guard."""
import subprocess
import unittest
from unittest.mock import patch

import install


class WindowsProcessGuardTests(unittest.TestCase):
    def check_output(self, output):
        with patch.object(install.os, 'name', 'nt'), patch.object(
            install.subprocess, 'check_output', return_value=output
        ) as query:
            install.require_closed()
        query.assert_called_once_with(
            ['tasklist', '/FI', 'IMAGENAME eq Dungeons-Win64-Shipping.exe',
             '/FO', 'CSV', '/NH'], text=True
        )

    def test_running_game_refused(self):
        with self.assertRaisesRegex(ValueError, 'Close Minecraft'):
            self.check_output('"Dungeons-Win64-Shipping.exe","1234","Console","1","200 K"\n')

    def test_process_name_case_insensitive(self):
        with self.assertRaisesRegex(ValueError, 'Close Minecraft'):
            self.check_output('"DUNGEONS-WIN64-SHIPPING.EXE","1234","Console","1","200 K"\n')

    def test_no_matching_process_allowed(self):
        self.check_output('INFO: No tasks are running which match the specified criteria.\n')

    def test_failed_process_query_does_not_allow_mutation(self):
        with patch.object(install.os, 'name', 'nt'), patch.object(
            install.subprocess, 'check_output', side_effect=subprocess.CalledProcessError(1, 'tasklist')
        ), self.assertRaises(subprocess.CalledProcessError):
            install.require_closed()


if __name__ == '__main__':
    unittest.main()
