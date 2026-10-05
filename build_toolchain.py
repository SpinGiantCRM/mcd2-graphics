"""Compiler-specific spelling of the same mandatory native stack-frame limit."""
import subprocess


def normalize_windows_header(data):
    """Normalize the copied SDK header without a case-only alias file."""
    return data.replace(b'#include <Windows.h>', b'#include <windows.h>')


def native_frame_guard(compiler):
    version = subprocess.run([str(compiler), '--version'], check=True,
                             capture_output=True, text=True).stdout.lower()
    if 'clang' in version:
        return ['-Wframe-larger-than=16384', '-Werror=frame-larger-than']
    if 'gcc' in version or 'g++' in version or 'free software foundation' in version:
        return ['-Werror=frame-larger-than=16384']
    raise ValueError('Unrecognized native compiler: cannot enforce the 16 KiB frame limit.')
