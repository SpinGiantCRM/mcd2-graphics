"""Read-only shipping-build comparison. Never enables a store or changes files."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

# Boundaries already checked by the Steam latency implementation. Matching these
# few bytes is evidence for investigation, not approval to call the Store ABI.
BOUNDARIES = {
    'simulationWrapper': (0x45546f0, bytes.fromhex('48 89 5c 24 08 48 89 74 24 10 57 48 83 ec 30')),
    'simulationStartCall': (0x4554769, bytes.fromhex('ff 50 38')),
    'simulationEndCall': (0x4554679, bytes.fromhex('ff 50 40')),
    'pacingCall': (0x4557b8a, bytes.fromhex('ff 50 30')),
    'modularRegistryGetter': (0x12a64d0, bytes.fromhex('48 83 ec 28 8b 0d')),
}


def read_exact(stream, offset, count, size):
    if offset < 0 or count < 0 or offset + count > size:
        raise ValueError('Invalid PE range')
    stream.seek(offset)
    data = stream.read(count)
    if len(data) != count:
        raise ValueError('Short PE range')
    return data


def pe_metadata(stream, size):
    dos = read_exact(stream, 0, 64, size)
    if dos[:2] != b'MZ':
        raise ValueError('Readable PE unavailable')
    nt = struct.unpack_from('<I', dos, 60)[0]
    header = read_exact(stream, nt, 24, size)
    if header[:4] != b'PE\0\0':
        raise ValueError('Readable PE unavailable')
    machine, count, timestamp = struct.unpack_from('<HHI', header, 4)
    optional_size = struct.unpack_from('<H', header, 20)[0]
    if not 1 <= count <= 96 or not 64 <= optional_size <= 4096:
        raise ValueError('Unsupported PE layout')
    optional = read_exact(stream, nt + 24, optional_size, size)
    if struct.unpack_from('<H', optional)[0] != 0x20b or machine != 0x8664:
        raise ValueError('Expected AMD64 PE32+')
    image_size = struct.unpack_from('<I', optional, 56)[0]
    sections = []
    section_hashes = {}
    for index in range(count):
        section = read_exact(stream, nt + 24 + optional_size + index * 40, 40, size)
        name = section[:8].split(b'\0')[0]
        virtual_size, rva, raw_size, raw_offset = struct.unpack_from('<IIII', section, 8)
        if raw_offset + raw_size > size or rva + virtual_size > image_size:
            raise ValueError('Invalid PE section')
        sections.append((rva, virtual_size, raw_size, raw_offset))
        if name == b'.text':
            # Hash only; never emit code or extract the executable.
            digest = hashlib.sha256()
            stream.seek(raw_offset)
            remaining = raw_size
            while remaining:
                block = stream.read(min(remaining, 1024 * 1024))
                if not block:
                    raise ValueError('Short code section')
                digest.update(block)
                remaining -= len(block)
            section_hashes['textSHA256'] = digest.hexdigest()
    matches = {}
    for name, (rva, expected) in BOUNDARIES.items():
        value = None
        for start, virtual_size, raw_size, raw_offset in sections:
            delta = rva - start
            if 0 <= delta and delta + len(expected) <= min(virtual_size, raw_size):
                value = read_exact(stream, raw_offset + delta, len(expected), size)
                break
        matches[name] = value == expected
    return {'architecture': 'AMD64', 'imageSize': image_size,
            'sectionCount': count, 'PETimeDateStamp': timestamp,
            **section_hashes, 'onDiskSteamBoundaryMatches': matches,
            'diskSignaturesEstablishRuntimeABI': False}


def file_version(path):
    if os.name != 'nt':
        return None
    env = dict(os.environ, MCD2_PROBE_EXE=str(path.resolve()))
    code = '$ErrorActionPreference="Stop";[System.Diagnostics.FileVersionInfo]::GetVersionInfo($env:MCD2_PROBE_EXE).FileVersion'
    try:
        result = subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive', '-Command', code],
                                env=env, capture_output=True, text=True, timeout=10, check=True)
        value = result.stdout.strip()
        # Versions only; do not emit arbitrary resource text or paths.
        return value if value and len(value) <= 64 and all(c in '0123456789., -' for c in value) else None
    except (OSError, subprocess.SubprocessError):
        return None


def inspect(path, expected_sha):
    result = {'filename': path.name, 'fileVersion': file_version(path),
              'readOnly': True, 'StoreSupportEnabled': False, 'runtimeQualified': False}
    try:
        with path.open('rb') as stream:
            size = os.fstat(stream.fileno()).st_size
            sha = hashlib.file_digest(stream, 'sha256').hexdigest()
            result.update(fileSize=size, SHA256=sha, exactVerifiedSteamBuild=sha == expected_sha)
            try:
                result['PE'] = pe_metadata(stream, size)
                result['PEReadStatus'] = 'readable'
            except (ValueError, struct.error):
                result['PEReadStatus'] = 'unavailable_or_unsupported'
    except OSError:
        result['fileReadStatus'] = 'unavailable'
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    if args.exe.resolve() == args.output.resolve():
        parser.error('Output must be a separate new JSON file')
    if args.output.exists():
        parser.error('Existing output retained; choose a new JSON filename')
    lock = json.loads((Path(__file__).resolve().parents[2] / 'dependencies.lock.json').read_text())
    report = inspect(args.exe, lock['game']['exeSHA256'])
    with args.output.open('x', encoding='utf-8') as stream:
        stream.write(json.dumps(report, indent=2) + '\n')
    print('Sanitized comparison saved; no executable, resources, saves or permissions changed.')


if __name__ == '__main__':
    main()
