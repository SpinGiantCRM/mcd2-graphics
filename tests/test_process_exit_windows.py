"""Windows-only real DLL-unload/process-exit query fixture, with no vendor code."""
from pathlib import Path
import os
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    if os.name != 'nt':
        raise SystemExit('Run this fixture on Windows.')
    with tempfile.TemporaryDirectory(prefix='mcd2-process-exit-') as directory:
        folder = Path(directory)
        source = ROOT / 'tests/process_exit_windows_probe.cpp'
        dll, exe, result = (folder / name for name in ('exit-probe.dll', 'exit-probe.exe', 'result.bin'))
        common = ['g++', '-std=c++20', '-static', '-Wall', '-Wextra', '-Werror', str(source)]
        subprocess.run(common + ['-shared', '-DMCD2_EXIT_PROBE_DLL', '-o', str(dll)], check=True)
        subprocess.run(common + ['-municode', '-o', str(exe)], check=True)
        subprocess.run([str(exe), str(dll), str(result)], check=True, timeout=20)
        assert result.read_bytes() == struct.pack('<4I', 0, 0, 1, 1), 'Incorrect NT termination query'
    print('Real NT query: ordinary DLL unload false, process termination true.')


if __name__ == '__main__':
    main()
