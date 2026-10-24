"""Exercise the shipped workflow gates without runners, network or game access."""
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


def workflow_bash(platform=None):
    if (platform or sys.platform) != 'win32':
        return shutil.which('bash')
    # Windows' PATH can resolve bare bash to the WSL launcher. Find Bash in
    # the Git installation instead, matching the workflow's shell provider.
    git = shutil.which('git')
    if git:
        for parent in Path(git).resolve().parents:
            candidate = parent / 'bin/bash.exe'
            if candidate.is_file():
                return str(candidate)
    return None


BASH = workflow_bash()


class ShellSelectionTests(unittest.TestCase):
    def test_windows_uses_git_bash_even_when_bare_bash_is_wsl(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / 'Program Files/Git'
            bash = root / 'bin/bash.exe'
            bash.parent.mkdir(parents=True)
            bash.touch()
            for location in ('cmd/git.exe', 'mingw64/bin/git.exe'):
                git = root / location
                git.parent.mkdir(parents=True, exist_ok=True)
                git.touch()
                with self.subTest(location=location), patch.object(
                    shutil, 'which', side_effect=lambda name: str(git) if name == 'git' else 'WSL/bash.exe'
                ):
                    self.assertEqual(Path(workflow_bash('win32')), bash.resolve())

    def test_windows_does_not_fall_back_to_wsl_without_git_bash(self):
        with patch.object(shutil, 'which', side_effect=lambda name: None if name == 'git' else 'WSL/bash.exe'):
            self.assertIsNone(workflow_bash('win32'))


def step_body(workflow, name):
    # Refuse a changed YAML shape instead of testing a copied or different gate.
    match = re.search(r"      - name: " + re.escape(name) +
                      r"\n(?:(?!      - |  [a-z]).*\n)*?        run: \|\n"
                      r"((?:          .*\n|\n)+)", workflow)
    if not match:
        raise AssertionError(f"Missing literal run block: {name}")
    return textwrap.dedent(match[1])


class NativeGateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        workflow = (ROOT / '.github/workflows/fg-windows-candidate.yml').read_text()
        body = step_body(workflow, 'Require the selected build to succeed')
        cls.gate = body.split("<<'PYTHON'\n", 1)[1].rsplit('PYTHON', 1)[0]
        compile(cls.gate, '<native-gate>', 'exec')

    def run_gate(self, selection='true', scope='success', build='success'):
        needs = {'changes': {'result': scope, 'outputs': {}},
                 'build': {'result': build, 'outputs': {}}}
        if selection is not None:
            needs['changes']['outputs']['native'] = selection
        env = dict(os.environ, NEEDS_JSON=json.dumps(needs))
        return subprocess.run([sys.executable, '-c', self.gate], env=env,
                              capture_output=True, text=True, timeout=10)

    def test_native_success_and_explicit_documentation_skip(self):
        self.assertEqual(self.run_gate().returncode, 0)
        self.assertEqual(self.run_gate('false', build='skipped').returncode, 0)

    def test_scope_failure_or_cancellation_cannot_pass(self):
        for scope in ('failure', 'cancelled', 'skipped'):
            with self.subTest(scope=scope):
                self.assertNotEqual(self.run_gate(scope=scope).returncode, 0)

    def test_required_build_cannot_fail_cancel_or_skip(self):
        for result in ('failure', 'cancelled', 'skipped'):
            with self.subTest(result=result):
                self.assertNotEqual(self.run_gate(build=result).returncode, 0)

    def test_missing_or_invalid_selection_cannot_authorize_skip(self):
        for selection in (None, '', 'TRUE', 'unknown'):
            with self.subTest(selection=selection):
                self.assertNotEqual(self.run_gate(selection, build='skipped').returncode, 0)

    def test_documentation_selection_requires_actual_skip(self):
        for result in ('success', 'failure', 'cancelled'):
            with self.subTest(result=result):
                self.assertNotEqual(self.run_gate('false', build=result).returncode, 0)


@unittest.skipUnless(BASH, 'Git Bash required by Windows CI')
class WindowsFailFastTests(unittest.TestCase):
    def test_early_test_failure_stops_actual_recipe_block(self):
        workflow = (ROOT / '.github/workflows/portable-checks.yml').read_text()
        windows = workflow.split('  windows-installer-checks:\n', 1)[1].split(
            '\n  guided-installer-checks:', 1)[0]
        self.assertRegex(windows, r'defaults:\s+run:\s+shell: bash')
        self.assertEqual(windows.count('\n    defaults:'), 1,
                         'Duplicate job keys invalidate the hosted workflow')
        body = step_body(windows, 'Native build recipe regressions')
        # The first disposable module fails; the second records execution if
        # that failure is masked. Use the workflow's real command sequence.
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'tests').mkdir()
            # Every earlier discovery command needs a real passing test:
            # newer Python versions fail discovery when no tests are found.
            # Derive the stand-ins from the real recipe so adding a check
            # cannot make this fixture stop before its injected failure.
            for pattern in re.findall(r'-p "(test_[^"*]+\.py)"', body):
                (root / 'tests' / pattern).write_text(
                    'import unittest\nclass Pass(unittest.TestCase):\n'
                    ' def test_pass(self): pass\n')
            (root / 'tests/test_build_toolchain.py').write_text(
                'import unittest\nclass Failure(unittest.TestCase):\n'
                ' def test_failure(self): self.fail("injected regression")\n')
            (root / 'tests/test_amd_latency_build.py').write_text(
                'from pathlib import Path\nPath("continued").touch()\n')
            executable = shlex.quote(Path(sys.executable).as_posix())
            body = body.replace('python ', executable + ' ')
            result = subprocess.run(
                [BASH, '--noprofile', '--norc', '-e', '-o', 'pipefail', '-c', body],
                cwd=root, capture_output=True, text=True, timeout=20)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('injected regression', result.stdout + result.stderr)
            self.assertFalse((root / 'continued').exists())


if __name__ == '__main__':
    unittest.main()
