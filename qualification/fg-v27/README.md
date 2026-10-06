# Windows FG candidate v27

[Download the candidate](mcd2-fg-dlss-windows-candidate-v27.zip).

SHA-256: `4ed1fc49f6dbeab02daff06604372ab1a73d06e7976ad7ba66fffc4c4d77314c`.

This is a reversible test candidate for PR10. It is not a public release and
includes no vendor runtimes, game assets, player saves or authentication files.
Start from the working Steam rc.1 installation; use the separately pinned
Streamline 2.14.1 runtime and qualified ReShade PR435 binary.

Follow the bundled `experiments/fg-streamline/README.md` and
`WINDOWS_FG_CLEARANCE_2026-10-06.md`. Supply `probe/`, `latency/`, `sr/` and
`ui/Pak/` to `game_trial.py`, with a fresh backup. Set `NativeUIToggle=1` as
instructed. Verify current-process source/FG state, preset changes and normal
shutdown. Restore the backup after testing.

[Linux performance comparison](../../experiments/fg-streamline/LINUX_FG_PERFORMANCE_2026-10-06.md).
Fresh-launch DLAA and DLAA after a Quality switch must be tested separately.
AMD/Intel fallback cannot qualify NVIDIA frame generation.

The workflow's separate folder-installer artifact tests packaging of the rc.1
payload; it does not install FG. Keep the entire extracted installer folder
together and qualify its exact downloaded hash. Neither candidate is signed or
cleared for publication. [Release gates](../../docs/NEXUS_RELEASE_GATE.md).
