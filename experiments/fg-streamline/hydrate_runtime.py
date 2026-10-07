"""Add separately acquired pinned SDK runtimes to a local test probe folder."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', required=True, type=Path)
    parser.add_argument('--probe', required=True, type=Path)
    args = parser.parse_args()
    receipt = json.loads((args.probe / 'build-receipt.json').read_text())
    lock = json.loads((Path(__file__).resolve().parents[2] / 'dependencies.lock.json').read_text())
    expected = lock['dependencies']['Streamline']['archiveSHA256']
    if receipt['archiveSHA256'] != expected or hashlib.sha256(args.archive.read_bytes()).hexdigest() != expected:
        raise ValueError('Unpinned SDK archive')
    staged = {}
    with zipfile.ZipFile(args.archive) as archive:
        for name, sha in receipt['files'].items():
            if Path(name).name != name or name not in (
                'sl.interposer.dll', 'sl.common.dll', 'sl.pcl.dll', 'sl.reflex.dll',
                'sl.dlss_g.dll', 'nvngx_dlssg.dll', 'NvLowLatencyVk.dll'):
                raise ValueError('Unexpected SDK runtime')
            data = archive.read('bin/x64/' + name)
            if hashlib.sha256(data).hexdigest() != sha:
                raise ValueError('Runtime does not match candidate')
            if (args.probe / name).exists():
                raise ValueError('Existing runtime retained: ' + name)
            staged[name] = data
    for name, data in staged.items():
        with (args.probe / name).open('xb') as stream:
            stream.write(data)
    print('Pinned production runtimes verified and extracted locally; game unchanged.')


if __name__ == '__main__':
    main()
