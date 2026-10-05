# Update 2 candidate Windows follow-up — 5 October 2026

PR7's exact-artifact Windows/AMD results are in [the Update 2 Windows report](UPDATE_2_WINDOWS_VALIDATION_2026-10-05.md) and [aggregate record](../qualification/update2/windows-checks.json). Installation, AMD fallback and three bounded clean exits passed. Required NVIDIA, HDR-output and input checks remain outstanding; Update 2 is still unpublished and release-blocked. These results do not revise the historical preview evidence below.

# Preview.2 qualification update — 5 October 2026

Preview.2 uses the exact PR6 addon and unchanged preview.1 UI/shader. Latest Linux runtime and install/removal evidence is in [the CachyOS report](LINUX_CACHYOS_REGRESSION_2026-10-05.md). Exact-artifact Windows AMD evidence, including the unexplained shutdown exception, is in [the Windows report](WINDOWS_REGRESSION_PR6_2026-10-05.md). Windows shutdown is not fully qualified. Additional Windows isolation has been deferred to the upcoming separate Reflex qualification; it is not reported as passed.

The earlier records below describe their specific artifacts and environments. They do not supersede these latest results. The original preview.1 tag and archive are unchanged.

# Preview validation — 4 October 2026

For the subsequent Windows/AMD investigation and fixed local candidate, see [Windows validation](WINDOWS_VALIDATION_2026-10-05.md). The results below apply to the original preview.1 payload.

## Qualified release gates

- UI and native builds completed with developer file controls disabled. Both stable interop adapters remained unchanged.
- The public parameterized source-build recipe completed on the local Linux build host. Its output is not substituted for the separately qualified release payload.
- 2,269 settings/runtime semantic and truncation checks passed, including optional context flags, invalid ranges and malformed framing.
- At 4K, keyboard changes clamp to 17%; extra decreases at the floor do not create new revisions. A mouse drag beyond the left edge clamps to 17%. The slider fill and thumb share the normalized value. Mouse preset changes and keyboard navigation passed; headers are skipped.
- The resolution arithmetic gives minima of 17% at 3840×2160, 25% at 2560×1440, 33% at 1920×1080 and 50% at 1280×720. Non-4K live resolution changes were not qualified in this release run.
- The exact packaged files were installed into the real game folder with a fresh mod-only profile: Native default, Quality/67%, remembered Custom77%. Existing game/account saves were not reset.
- Main-menu DLSS selection stayed effectively Native. Main-menu → gameplay activated Quality at 2560×1440 → 3840×2160. Movement and jumping remained playable.
- Quality → Native reached the current revision acknowledgement and restored 100% native source scale.
- DLAA ran at 3840×2160 → 3840×2160. Returning to Quality succeeded.
- Custom17% ran at 656×368 → 3840×2160. Returning to Quality succeeded. Source dimensions include the game's alignment; the scalar is 17%.
- Gameplay → main menu retained Quality intent and completed the effective Native acknowledgement. Normal desktop quit succeeded.
- Real uninstall removed the recorded payload and retained dependencies, configuration and own preference slots byte for byte. Reinstall and all six installed file hashes passed; original user preferences were restored. Seven isolated filesystem gates also passed, including duplicate-install refusal and retention of modified own files.

The archived UE Extended dependency is **nightly-20260928**, downloaded from its official release and checked against the published SHA-256. The older development snapshot is not the dependency shipped in this lock file.

See numeric summaries in [runtime-validation.json](runtime-validation.json), [actual-install-validation.json](actual-install-validation.json) and [install-validation.json](install-validation.json). Five sampled feature generations recorded zero NGX/contract failures. The first Quality generation recorded two transient native fallbacks and recovered; uninterrupted DLSS operation across every frame is not claimed. No raw runtime logs, account data, screenshots containing party codes, dumps or extracted game shaders are published.

## Release-blocking issue handled

An earlier candidate enabled DLSS in the rendered frontend and hung during entry to gameplay (the game reported a render-thread timeout). That candidate is excluded. The release preserves the chosen preset while keeping the verified frontend map Native. Gameplay activation requires a current-process runtime handshake; stale persisted gameplay flags cannot authorize another process. Both an existing saved DLSS profile and the packaged fresh profile reached gameplay with this guard. The original hang's precise GPU/driver cause is not established, and these bounded checks are not proof that all hangs are eliminated.

A Native change in an earlier run with the unavailable development dependency had not completed its acknowledgement before exit. The archived release dependency's Quality → Native acknowledgement and subsequent DLAA/Quality transitions passed. The ownership gate still deliberately waits for safe history reset and command-list retirement; do not interpret a saved selection as active until its status acknowledges it.

## Unqualified

Windows; physical controller; split-screen; unusual aspect ratios; device recreation/removal; long sessions; death/respawn; all cinematics and transitions; comprehensive temporal/perceptual parity. Older prototype travel results do not automatically qualify every packaged-build lifecycle case. HDR brightness values are decoded signals from prior work, not measurements of physical panel output or proof of an official HDR master. This preview adds no ray tracing, ray reconstruction, frame generation or Reflex.
