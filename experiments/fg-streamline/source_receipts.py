"""Canonical source keys for receipts produced on either build host."""
import hashlib


def provider_sources(repository):
    return [path.relative_to(repository).as_posix()
            for path in sorted((repository / 'src/providers').glob('*.hpp'))]


def source_hashes(repository, experiment, names):
    result = {}
    for raw in names:
        name = raw.replace('\\', '/')
        root = repository if name.startswith('src/') else experiment
        result[name] = hashlib.sha256((root / name).read_bytes()).hexdigest()
    return result
