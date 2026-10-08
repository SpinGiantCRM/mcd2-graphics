"""Record canonical Git inputs before provider refactoring; never read player data."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess

RELEASE_SOURCE = "e421e57e7667f0e9ec71e1940c6daa352a39f0c7"
SNAPSHOT = "qualification/providers/released-baseline.json"
ROOTS = ("src/", "tools/", "experiments/fg-streamline/")
SUFFIXES = {".cpp", ".hpp", ".h", ".cs", ".csproj", ".hlsl", ".def", ".ini", ".py", ".json", ".cmake", ".txt"}
FILES = {"manifest.json", "dependencies.lock.json", "install.py", "build.py",
         "build_latency.py", "build_installer.py", "build_toolchain.py"}


def git(repo: Path, *args: str) -> bytes:
    return subprocess.check_output(["git", "-C", str(repo), *args], stderr=subprocess.PIPE)


def blob(repo: Path, ref: str, path: str) -> bytes:
    return git(repo, "show", f"{ref}:{path}")


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def source_hashes(repo: Path, ref: str) -> dict[str, str]:
    commit = git(repo, "rev-parse", "--verify", "--end-of-options", f"{ref}^{{commit}}").decode().strip()
    paths = git(repo, "ls-tree", "-r", "--name-only", "-z", commit).decode("utf-8").split("\0")
    selected = [p for p in paths if p in FILES or
                (p.startswith(ROOTS) and Path(p).suffix in SUFFIXES)]
    return {p: sha(blob(repo, commit, p)) for p in sorted(selected)}


def record(repo: Path, ref: str | None = None) -> dict:
    commit = git(repo, "rev-parse", "--verify", "--end-of-options", f"{ref or RELEASE_SOURCE}^{{commit}}").decode().strip()
    manifest = json.loads(blob(repo, commit, "manifest.json"))
    lock_bytes = blob(repo, commit, "dependencies.lock.json")
    lock = json.loads(lock_bytes)
    return {
        "schema": 1,
        "release": manifest["version"],
        "implementationCommit": commit,
        "sourceHashEncoding": "SHA-256 of canonical Git blob bytes, not checkout line endings",
        "sourceFiles": source_hashes(repo, commit),
        "ownedPayload": manifest["files"],
        "game": lock["game"],
        "dependencyLockSHA256": sha(lock_bytes),
        "runtimeQualification": "Historical receipts only; recording hashes is not a new runtime test",
    }


def verify(repo: Path, snapshot: dict, ref: str) -> dict:
    # Rebuild the entire frozen record, including its file set. Edited pins,
    # omissions and added entries cannot silently redefine the old baseline.
    if snapshot != record(repo):
        raise ValueError("Baseline differs from the frozen implementation commit")
    before = snapshot["sourceFiles"]
    current = source_hashes(repo, ref)
    return {
        "baseline": RELEASE_SOURCE,
        "comparisonCommit": git(repo, "rev-parse", "--verify", "--end-of-options", f"{ref}^{{commit}}").decode().strip(),
        "added": sorted(current.keys() - before.keys()),
        "removed": sorted(before.keys() - current.keys()),
        "changed": sorted(p for p in before.keys() & current.keys() if before[p] != current[p]),
        "newRuntimeQualification": False,
    }


def verify_payload(root: Path, expected: dict[str, str]) -> list[str]:
    mismatches = []
    base = root.resolve(strict=True)
    for name, digest in sorted(expected.items()):
        relative = Path(name)
        if relative.is_absolute() or ".." in relative.parts or "\\" in name or ":" in name:
            raise ValueError("Invalid payload member")
        path = base / relative
        # Check containment before reading; report relative names only.
        if not path.resolve().is_relative_to(base):
            raise ValueError("Payload member escapes its root")
        if not path.is_file() or sha(path.read_bytes()) != digest:
            mismatches.append(name)
    return mismatches


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("record", "verify"))
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--snapshot", type=Path)
    parser.add_argument("--compare-ref", default="HEAD")
    parser.add_argument("--require-unchanged", action="store_true")
    parser.add_argument("--payload-root", type=Path,
                        help="Optional expanded owned payload; no saves or logs are read")
    args = parser.parse_args()
    path = args.snapshot or args.repo / SNAPSHOT
    try:
        if args.action == "record":
            # A record is created once. Updating a reference requires an explicit
            # new file rather than overwriting the protected baseline.
            value = record(args.repo)
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open("x", encoding="utf-8", newline="\n") as out:
                out.write(json.dumps(value, indent=2, sort_keys=True) + "\n")
            print(json.dumps({"recordedSourceFiles": len(value["sourceFiles"]),
                              "ownedPayloadFiles": len(value["ownedPayload"])}))
            return 0
        value = json.loads(path.read_text(encoding="utf-8"))
        report = verify(args.repo, value, args.compare_ref)
        if args.payload_root:
            report["payloadMismatches"] = verify_payload(args.payload_root, value["ownedPayload"])
        print(json.dumps(report, indent=2, sort_keys=True))
        if report.get("payloadMismatches"):
            return 1
        return int(args.require_unchanged and any(report[k] for k in ("added", "removed", "changed")))
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        # Do not emit arbitrary local paths or subprocess output into receipts.
        print(json.dumps({"error": type(error).__name__, "verified": False}))
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
