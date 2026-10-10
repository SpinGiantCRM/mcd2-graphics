"""Materialize the own-source include closure for an isolated SR build."""
from pathlib import Path
import shutil


def copy_sr_sources(repo: Path, source: Path) -> None:
    for directory in ('src/native', 'src/providers'):
        shutil.copytree(repo / directory, source / directory, dirs_exist_ok=True)
    # Same terminal query/lifetime guard as the latency addon; no vendor files.
    (source / 'src/latency').mkdir(parents=True, exist_ok=True)
    for name in ('process_exit.hpp', 'process_lifetime.hpp'):
        shutil.copyfile(repo / 'src/latency' / name, source / 'src/latency' / name)
    # Plain C ABI only. Vendor SDK headers and runtime DLLs remain external.
    abi = Path('experiments/providers/fsr_game_bridge.h')
    (source / abi).parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(repo / abi, source / abi)
    for name in ('build.py', 'build_toolchain.py'):
        shutil.copyfile(repo / name, source / name)

    for name in ('fg_camera_contract.h', 'fg_camera_math.h'):
        relative = Path('experiments/fg-streamline') / name
        (source / relative).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(repo / relative, source / relative)
