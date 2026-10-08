# Provider architecture: baseline and migration boundaries

## Baseline (PR-A)

The starting runtime is `0.3.0-preview.1`, implementation commit
`e421e57e7667f0e9ec71e1940c6daa352a39f0c7`. The roadmap update at
`23189ad494c6344099894b096003c571d35b1952` changes documentation only.

[The frozen record](../qualification/providers/released-baseline.json) pins
81 canonical source inputs, all 12 owned payload hashes, game identity and the
dependency lock. Git blob bytes avoid Windows checkout line-ending differences.
The record is separate from future candidate manifests and cannot be overwritten
by the recording command.

```sh
python tools/provider_baseline.py verify
python tools/provider_baseline.py verify --compare-ref YOUR_CANDIDATE_COMMIT
```

Verification rebuilds the reference from the pinned commit, rejects edited or
incomplete records, and reports added, removed and changed candidate inputs.
Changes are reported for review, rather than prohibited: subsequent provider PRs
must change some of these files. `--require-unchanged` is available for controls.
An optional `--payload-root` verifies only manifest-owned files in an expanded
payload; it does not read player saves, logs or account data.

This is provenance verification, not a fresh gameplay, performance or shutdown
qualification. Preserve the existing [Linux receipt](LINUX_FINAL_QUALIFICATION_2026-10-07.md),
[Windows AMD receipt](WINDOWS_FULL_QUALIFICATION_2026-10-07.md) and
[benchmark](BENCHMARK_2026-10-07.md). Windows AMD fallback does not qualify active
Windows NVIDIA rendering. Historical shutdown failures and delayed clean exits
remain part of the regression scope.

## Existing integration boundaries

| Boundary | Current source | Requirement for extraction |
|---|---|---|
| Saved SR intent | `src/ui/ModActor.cs`, `src/native/settings_bridge.hpp` | Preserve v4 mode, preset, exact source ratio, last custom value and fallback preference. Current native decoder skips the last-custom scalar; migration must read it explicitly. |
| Separate FG/display saves | `src/ui/ModActor.cs`, `src/latency/display_protocol.hpp` | Preserve FG On/Off, HDR calibration and Reflex preference with their separate revisions. Runtime acknowledgements are not saved feature availability. |
| Startup DXGI owner | `experiments/fg-streamline/bootstrap_dxgi.cpp` | Resolve saved FG presentation owner before the first shipping swapchain. A menu-only provider switch is insufficient. |
| Source-scale transaction | `src/native/ui_control_present.hpp`, `src/native/upscaler_support.hpp` | Keep rendering-adapter rejection before source reduction, the 15-second acknowledgement deadline and Native rollback. |
| Temporal source and NGX evaluation | `src/native/observer.cpp`, `src/native/ngx_lean.hpp`, `src/shaders/live_dense.hlsl` | Extract observation/conversion from evaluation; do not run NGX to obtain AMD/Intel guides or install a second temporal interceptor. |
| Recording/GPU retirement | `src/native/lean_borrow_lifetime.hpp` | Keep successful-Reset proof, replay ownership and later completed fences. A reference count alone cannot prevent texture overwrite. |
| FG world/UI observation | `experiments/fg-streamline/guide_recon.cpp`, `fg_alpha_copy.h` | Match the same real frame as source guides; current capture retains UI alpha, not independently captured full UI RGBA. |
| SDK boundary and shared NGX owner | `experiments/fg-streamline/streamline_probe_bridge.cpp`, `fg_bridge_contract.h` | Preserve the existing MSVC ABI bridge and shared NGX teardown; no SDK calls while holding the observer mutex. |
| Frame tokens and pacing | `src/latency/token_coordinator.hpp`, `src/latency/latency_addon.cpp` | Preserve pre-input pacing, actual marker boundaries and matching engine-frame tokens. Resolve one compatible pacing owner. |

## Next slices

1. **PR-B:** portable requested-setting types and validated v4 migration, then
   versioned persistence/startup handoff. Keep legacy files until a complete new
   record is written and accepted. Separate intent from effective capabilities.
2. **PR-C:** typed pre-SR, reconstructed-world and presentation packets with
   per-consumer leases. First prove Native guide capture without NGX and unchanged
   DLSS output; then move existing consumers behind those boundaries.
3. **PR-D/E:** exclusive presentation/latency routing and an isolated AMD SDK
   capability probe. Inspect the actual rendering adapter and D3D12 runtime.
4. **PR-F onward:** FSR SR, then analytical FG/Anti-Lag and both separately
   qualified cross-provider pairings.

The shared architecture must preserve provider-specific motion, exposure, colour,
UI and frame-counter contracts. Keep instrumentation separate from performance
runs. SDK output counts do not establish physical presentation or input latency.
MFG, denoising and late reprojection interfaces remain later gated work.

No new runtime feature, settings migration or installed-game change is performed
by the baseline tool. Native, UI, shader, installer and dependency files remain
at the released baseline for this slice.

## Requested-setting contract (PR-B, first slice)

`src/providers/graphics_intent.hpp` introduces portable, SDK-independent types
for SR selection, per-provider quality preferences, FG provider/strategy/count,
latency preference and HDR calibration. They describe requests; they do not
declare availability or create a device, feature, proxy or pacing handler.

The in-memory v4 migration preserves:

- Native versus NVIDIA selection and the preferred NVIDIA preset while Native
  is selected;
- current and last-custom values, plus exact NVIDIA Quality `2/3` and Ultra
  Performance `1/3` ratios rather than rounded UI percentages;
- FG On/Off as the legacy NVIDIA single-interpolation request;
- Reflex mode, HDR values, fallback preference and all three save revisions.

Previous runtime/context acknowledgements are deliberately excluded from the
new requested-setting type. Inputs must first pass the appropriate legacy save
framing/semantic decoder; the migration additionally validates their combination.
Failure leaves the destination untouched. It never writes or deletes a save.

The startup helper returns only a **requested** presentation owner. The next
router must independently approve its SDK/device, inputs, revision and lifetime
before wrapping a shipping swapchain. Structural bounds on scales and MFG counts
are format limits, not claims that any provider supports them.

This first slice is not included by the shipping addons, UI or bootstrap. Linux
and Windows CI compile the portable test; this does not qualify vendor runtime
execution on either system.

## Settings record and store (PR-B, second slice)

The new headers remain isolated from shipping addons, menus and the DXGI
bootstrap. They introduce a tested storage boundary for later integration:

- `legacy_graphics.hpp`: validates an immutable snapshot of the three mod-owned
  v4 SR / v1 display / v1 FG saves and migrates their requested preferences. It
  reads `LastCustomScaleBasisPoints` explicitly, refuses missing or malformed
  values, and excludes runtime/source/session acknowledgements. Zero-valued
  Unreal properties may be absent. Unknown properties and wrong field types
  are refused. No legacy file is written, renamed or deleted.
- `graphics_record.hpp`: encodes/decodes one fixed 128-byte little-endian v5
  record. The early requested owner and full menu intent share this record,
  avoiding a two-file commit. Invalid data leaves decoder output untouched;
  `selectStartup` returns an invalid Native request. CRC detects accidental
  corruption; it is not authentication or a capability check.
- `graphics_store.hpp`: publishes only to an explicitly supplied existing
  private directory. It has no game-path discovery or construction-time IO.
  The caller reserves `intent-v5.bin`, `intent-v5.pending` and `intent-v5.lock`
  there. Readers use only the committed record. Writers take a nonblocking OS
  lock, compare the expected revision **and checksum**, require a newer revision,
  flush and reread the pending file, then replace the committed file. A crash
  releases the lock; the next locked writer discards an unfinished pending file.
  Corrupt committed data is refused, not silently overwritten from legacy saves.

### Wire layout, version 1

All integers are unsigned 32-bit little-endian. Booleans must be exactly 0 or 1.
Vendor enums are our request enums, never copied SDK constants.

| Byte offset | Field |
|---|---|
| 0–7 | `MCD2GI5` followed by a zero byte |
| 8 / 12 / 16 / 20 | Wire version 1 / length 128 / CRC-32/ISO-HDLC / requested owner |
| 24 / 28 / 32 | Intent schema 5 / revision / SR provider |
| 36–71 | NVIDIA, AMD, Intel preferences: quality / custom scale / last custom scale |
| 72 / 76 / 80 / 84 / 88 | Native fallback preference / FG enabled / FG provider / strategy / multiplier |
| 92 / 96 | Latency provider / mode |
| 100 / 104 / 108 / 112 | HDR enabled / peak / paper white / UI nits |
| 116 / 120 / 124 | Migrated SR / display / FG revisions; all zero for new settings |

CRC covers the complete record with bytes 16–19 treated as zero. Length, magic,
versions, enum/scalar ranges, boolean encoding, migration revisions and the
derived owner must all agree, even with a valid checksum. NVIDIA exact ratios
remain computed from its named preset rather than rounded UI percentages.

### Startup and later integration

`StartupSelection` pairs the validated requested route/stamp with a nonzero,
host-generated process/session identity. It is not saved. `matchesStartup`
requires the same session, revision, checksum and requested owner. A changed
menu record cannot claim that a different startup route is already active.
The later router must separately authorize capabilities and acknowledge safe
same-owner updates; this helper does not implement live settings changes or
hot switching. Keep unsupported requests as preferences while reporting Native
as the effective fallback.

Before wiring this into the live UI/bootstrap, implement and qualify:

1. A stable snapshot read of all three legacy slots, with all bytes rechecked
   before committing migration. Their independent revisions do not prove three
   reads were simultaneous.
2. One UI/native writer and an explicit recovery policy for invalid v5 data;
   retain the untouched legacy slots and never auto-downgrade an existing v5
   record. A stale or busy writer must reload rather than retry old settings.
3. An early bootstrap read before vendor initialization, actual adapter/SDK
   gating, one presentation owner and truthful runtime/restart acknowledgements.
4. Real menu, restart, persistence and platform regression tests with the rebuilt
   artifacts. The synthetic tests do not qualify gameplay or vendor activation.

### Filesystem guarantees and checks

Linux uses a flushed same-directory rename followed by directory `fsync`.
Windows uses wide-character handles, `FlushFileBuffers`, and same-directory
`SetFileInformationByHandle(FileRenameInfoEx)` with replacement/POSIX flags,
without a copy fallback or delete-before-move. This requires a filesystem and
Windows 10+ API supporting that operation; failure retains the old record.
Readers permit sharing for deletion. An already-open old handle remains a valid snapshot
after its filename is replaced; the reader accepts zero remaining links while
continuing to refuse multiple links. See Microsoft's [file sharing contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew),
[flush API](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers),
[rename API](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfileinformationbyhandle)
and [POSIX replacement semantics](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_rename_information).

The transaction targets trusted, local private storage, not a hostile same-user
writer or arbitrary network filesystem. Reparse points/symlinks, non-files and
multi-link committed files are refused. If directory sync fails after a Linux
replacement, `PublishedSyncUncertain` means the new record is already visible;
reload instead of claiming rollback. Power-loss/filesystem durability is not
established by process-termination tests.

Linux and Windows CI exercise synthetic Unicode paths, 144 provider request
combinations, 1,166 record rejection cases, independent Python/CRC wire bytes,
72 actual legacy-save combinations and 7,102 legacy framing/type/truncation
cases. Separate processes test lock contention, concurrent readers/writers,
abrupt termination before commit, pending-file recovery and unchanged legacy
files. No player saves, live preferences or installed binaries are read/changed.
