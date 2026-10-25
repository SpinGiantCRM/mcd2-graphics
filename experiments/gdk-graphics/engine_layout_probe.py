"""Read-only inspection of the running WinGDK engine; never installs hooks."""
import argparse
import ctypes
from ctypes import wintypes
import json
import re
from pathlib import Path
import struct


def pe_sections(header: bytes) -> tuple[int, int, list[tuple[str, int, int]]]:
    if len(header) < 64 or header[:2] != b'MZ':
        raise ValueError('Invalid DOS header')
    pe = struct.unpack_from('<I', header, 60)[0]
    if pe + 24 > len(header) or header[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Invalid PE header')
    machine, count, timestamp = struct.unpack_from('<HHI', header, pe + 4)
    optional = struct.unpack_from('<H', header, pe + 20)[0]
    table = pe + 24 + optional
    if machine != 0x8664 or optional < 60 or table + count * 40 > len(header):
        raise ValueError('Unsupported PE layout')
    image_size = struct.unpack_from('<I', header, pe + 24 + 56)[0]
    sections = []
    for index in range(count):
        pos = table + index * 40
        name = header[pos:pos + 8].rstrip(b'\0').decode('ascii')
        size, rva = struct.unpack_from('<II', header, pos + 8)
        if rva > image_size or size > image_size - rva:
            raise ValueError('Section outside image')
        sections.append((name, rva, size))
    return timestamp, image_size, sections


def inspect(pid: int) -> dict:
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    psapi = ctypes.WinDLL('psapi', use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.ReadProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p,
                                       ctypes.c_void_p, ctypes.c_size_t,
                                       ctypes.POINTER(ctypes.c_size_t)]
    kernel.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD,
                                                wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD)]
    psapi.EnumProcessModules.argtypes = [wintypes.HANDLE, ctypes.POINTER(ctypes.c_void_p),
                                        wintypes.DWORD, ctypes.POINTER(wintypes.DWORD)]
    handle = kernel.OpenProcess(0x410, False, pid)  # Query information and VM read only.
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        path = ctypes.create_unicode_buffer(32768)
        length = wintypes.DWORD(len(path))
        if not kernel.QueryFullProcessImageNameW(handle, 0, path, ctypes.byref(length)):
            raise ctypes.WinError(ctypes.get_last_error())
        if Path(path.value).name.lower() != 'dungeons-wingdk-shipping.exe':
            raise ValueError('Refusing a process other than the WinGDK game')
        modules = (ctypes.c_void_p * 1024)()
        needed = wintypes.DWORD()
        if not psapi.EnumProcessModules(handle, modules, ctypes.sizeof(modules), ctypes.byref(needed)):
            raise ctypes.WinError(ctypes.get_last_error())
        base = modules[0]

        def read(rva: int, size: int) -> bytes:
            buffer = ctypes.create_string_buffer(size)
            copied = ctypes.c_size_t()
            if not kernel.ReadProcessMemory(handle, base + rva, buffer, size, ctypes.byref(copied)):
                raise ctypes.WinError(ctypes.get_last_error())
            if copied.value != size:
                raise ValueError('Incomplete engine code read')
            return buffer.raw

        timestamp, image_size, sections = pe_sections(read(0, 4096))
        text_sections = [s for s in sections if s[0] == '.text']
        if len(text_sections) != 1:
            raise ValueError('Ambiguous executable section')
        _, rva, size = text_sections[0]
        text = read(rva, size)
        signature = bytes.fromhex('48895c24084889742410574883ec30')
        candidates = []
        pos = text.find(signature)
        while pos >= 0:
            gates = [(0x79, 'ff5038'), (-0x77, 'ff5040'), (0x349a, 'ff5030')]
            matches = [0 <= pos + delta <= len(text) - 3 and
                       text[pos + delta:pos + delta + 3] == bytes.fromhex(pattern)
                       for delta, pattern in gates]
            candidates.append({'rva': hex(rva + pos), 'historicalDispatchGates': matches})
            pos = text.find(signature, pos + 1)
        gated = [int(c['rva'], 16) for c in candidates if all(c['historicalDispatchGates'])]
        callers = []
        if len(gated) == 1:
            offset = text.find(b'\xe8')
            while 0 <= offset <= len(text) - 5:
                target = rva + offset + 5 + struct.unpack_from('<i', text, offset + 1)[0]
                if target == gated[0]:
                    callers.append(hex(rva + offset))
                offset = text.find(b'\xe8', offset + 1)
        registry = 0xaa7a270
        registry_contract = None
        if timestamp == 0xc4bf592a and image_size == 0xbc81000:
            vtable = struct.unpack('<Q', read(registry, 8))[0]
            if not base <= vtable <= base + image_size - 56:
                raise ValueError('Registry vtable outside engine image')
            entries = struct.unpack('<7Q', read(vtable - base, 56))
            if any(not base <= address < base + image_size for address in entries):
                raise ValueError('Registry entry outside engine image')
            registry_contract = {'registry': hex(registry), 'vtable': hex(vtable - base),
                                 'entries': [hex(address - base) for address in entries]}
        counter_references = []
        if registry_contract:
            pattern = rb'[\x48\x4c](?:\x8b|\x89|\xff)[\x05\x0d\x15\x1d\x25\x2d\x35\x3d].{4}'
            for instruction in re.finditer(pattern, text, re.DOTALL):
                offset = instruction.start()
                target = rva + offset + 7 + struct.unpack_from('<i', text, offset + 3)[0]
                if target in (0xadd9fd0, 0xadd9fd8):
                    counter_references.append({'instruction': hex(rva + offset),
                                               'target': hex(target), 'bytes': instruction[0].hex()})
        return {'timestamp': hex(timestamp), 'imageSize': hex(image_size),
                'simulationPrologues': candidates, 'simulationCallers': callers,
                'inspectedRegistryContract': registry_contract,
                'counterReferences': counter_references, 'hooksInstalled': False}
    finally:
        kernel.CloseHandle(handle)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', required=True, type=int)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    result = inspect(args.pid)
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    matching = sum(all(c['historicalDispatchGates']) for c in result['simulationPrologues'])
    print(json.dumps({'timestamp': result['timestamp'], 'imageSize': result['imageSize'],
                      'prologues': len(result['simulationPrologues']),
                      'historicalGateMatches': matching, 'hooksInstalled': False}))
