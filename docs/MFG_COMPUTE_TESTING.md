# Isolated MFG compute qualification

This experiment evaluates an externally acquired headless compute interface with
owned D3D12 images. The original probe is separate from the installer and fixed NVIDIA MFG path.
The dev.2 integration candidate adds a source-built game bridge and presenter;
its status and remaining game checks are recorded below.

The target remains software MFG on Radeon and NVIDIA, native provider MFG where
available, and NVIDIA midpoint plus software intermediate frames at 4x/6x. A
native NVIDIA presenter capability is insufficient: that mixed path needs an
actual caller-owned native intermediate on the same real-frame timeline. An FSR
midpoint combined with software output must not be described as NVIDIA plus AMD
generation. No GPU model name grants a capability.

## Build and inputs

`experiments/providers/build_mfg_compute_probe.py` compiles the probe and a
CPU-only NGX parameter ABI fixture against public NVIDIA declarations from
commit `374959484e79a640feaba44c93ac8cfb0a03f5b5`. Both use the Microsoft C++ ABI.
The native candidate artifact contains these own binaries, source/header/tool
hashes and the header license. CI runs the ABI fixture, not the GPU probe.
No Enabler or graphics-provider runtime is bundled.

```powershell
python experiments/providers/build_mfg_compute_probe.py `
  --ngx-headers C:/sdk/DLSS/include --output C:/trials/mfg-build
```

A cross build additionally accepts `--windows-sysroot`, `--clang-cl` and
`--linker`. Pinned headers are verified before compilation.

The inspected external input is [DLSS Enabler 4.9.0](https://www.nexusmods.com/site/mods/757?tab=files&file_id=9301).
Its archive contains `version.dll`, SHA-256
`f131734e9bf1fd191a9f60fad54231cd88e45ada3c957bd02c8763f8e9ad4440`.
It exposes `DLSSG_NVSDK_NGX_D3D12_*` entry points and phase index/count
parameters. Exports do not qualify behavior, prove an algorithm or establish
native NVIDIA output availability.

Acquire it directly under the author's terms. Do not bundle or patch it. The isolated probe does not install it in the game. The runner copies only this exact external input and our own
probe into a fresh directory, naming the module `dlss-enabler-headless.dll`.
The probe verifies its hash again and holds a file handle against replacement.
The external module stays loaded until process exit; live unload is not tested.

## Functional run

Close MCD2 first. Use a new directory per multiplier and platform:

```powershell
python experiments/providers/run_mfg_compute_probe.py `
  --build C:/trials/mfg-build --runtime C:/downloads/version.dll `
  --run-dir C:/trials/mfg-4x --multiplier 4
```

On Linux also pass `--proton /path/to/proton`, `--steam /path/to/Steam` and
`--prefix /path/to/existing-isolated-prefix`. Its `pfx` must already exist.
Never use Steam's game prefix or an Xodus game prefix. The run directory must be
fresh and outside a game tree. A running MCD2 shipping process refuses the run.
Execution is bounded to 180 seconds, with timeout and exit status recorded.

The probe owns its queue, textures, command allocator, fences and NGX parameter
object. Before each evaluation it declares `DLSSG.IsRecording=1`: the caller
owns the open list and its submission. The exact inspected interface must return
`DLSSG.FlushRequired=0`; otherwise the probe stops. Omitting the recording flag
allowed the external backend to reset/close the caller's list, causing a
reproducible command-state failure and heap corruption in the Linux control.
The Microsoft ABI fixture checks that every call resets this declaration.
It creates no swapchain and installs no hooks. The external module's
headless behavior remains part of qualification. Images are 1280×720 RGBA16F,
with owned depth and full-resolution current-to-previous pixel motion vectors.
Two warm-up frames precede ten checked real-frame pairs. These are functional
image checks, not an FPS or latency benchmark.

For each requested 2x–6x count, require:

- Actual hardware adapter identity, sufficient reported phase capability and
  successful initialization, creation and every phase evaluation.
- A different output resource per phase of the same real-frame pair. Generated
  frames never become real-history inputs.
- Completed queue fences before CPU readback and context/resource retirement.
- Finite, nonzero images, distinct per-phase hashes and increasing motion
  positions within one pixel of the requested time on the translating stripe
  pattern. Red-channel mean absolute fit error must be ≤0.12.
- Successful feature release and SDK shutdown, normal process exit, no timeout.

`mfg-evidence.jsonl` records actual API outcomes and image measurements.
`run-result.json` separates the functional gate from visual review and game
qualification. The last pair is saved as `previous-real.rgba16f`,
`current-real.rgba16f` and `phase-N-of-M.rgba16f`: little-endian half-float RGBA,
1280 pixels per row, no padding. Inspect phase order and content. Synthetic
conformance cannot establish game artifact quality or presentation pacing.
A failed fence or SDK release retains resources until process exit.

## Bounded Linux software-compute result

On 24 October 2026, the corrected probe passed all five requested counts on an
RTX 4080 SUPER, NVIDIA driver 615.78.08, CachyOS Proton
`cachyos-11.0-20261015-slr`, VKD3D-Proton 3.1.0. Across ten checked real pairs per
count, all 150 generated images were finite/nonzero, had distinct phase hashes
and correctly ordered stripe positions. Feature release, SDK shutdown and all
five process exits succeeded. The 6x phase strip was visually inspected against
the two owned real inputs. The external module was the exact pinned 4.9.0 DLL;
automatic Proton FSR4 replacement was disabled for these controls.

The final hosted candidate from [run 40079305728](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/40079305728)
was replayed after the per-call flush-response guard. All five counts passed;
all 150 image measurements and saved raw images matched the reviewed controls.
The [bounded qualification record](../qualification/providers/linux-mfg-compute-2026-10-24.json)
pins the exact probe, artifact, external module and environment.

This is a short synthetic compute result, not a game, Radeon, image-quality,
latency or presentation qualification. It does not establish the internal
algorithm/provider from a DLL filename. Native NVIDIA caller-owned output and
true mixed-provider generation remain separate gates. Preserve the failed
missing-recording-flag controls alongside the corrected runs.

## Qualification order

Complete Linux/NVIDIA game qualification first. The available Windows/Radeon
laptop is scheduled for an installer/startup check with MFG disabled afterward;
it cannot run the MFG matrix. Keep Windows MFG explicitly untested. A future
capable Radeon tester may run the isolated 2x–6x matrix independently, but the
current laptop launch check is not evidence for that path.

The exact external headless interface and provider attribution still need
confirmation before production admission. After compute proof, connect phase
outputs to one presentation owner and one latency owner, with per-phase
completion and current game identity. Qualify pause/resume, Off recovery,
travel, resizing, HDR/SDR and shutdown. Native NVIDIA output acquisition and
mixed-provider 4x/6x have their own acceptance gate. Performance benchmarks
remain a separate later activity.

## Native NVIDIA MFG helper trial

The integration branch's existing fixed NVIDIA MFG path uses Streamline's actual
reported maximum. An optional external compatibility helper can be investigated
on older RTX hardware; it must not become a GPU-name whitelist or permission to
force a production count above the reported limit.

The separately acquired [MFG Unlocker 1.6.1](https://github.com/dashdogy/MFG-Unlocker/releases/tag/v1.6.1)
archive has SHA-256
`abe4d6ae7eab1ea4cacc4775b05521c459516ed72f434784e4420080571e87c5`.
Its unmodified `MFGUnlocker.dll` has SHA-256
`24e3e6a8c07639fd9cbd79274e4eb21cc65a13f0453e1157720620c3d6286b33`.
It remains an external dependency, outside the installer and own-payload archive.
Follow the author's installation instructions and preserve unrelated loaders.

For a controlled game trial, use one documented proxy filename that the actual
shipping executable imports and whose adjacent slot is empty. The inspected
Steam executable imports `WINMM.dll`; a separately staged `winmm.dll` is the
planned route. Confirm those facts again before installation. Never replace the
existing graphics proxy or load a second helper copy. Preserve the original
launch options and remove only the still-owned helper file during restoration.
Do not alter global NVIDIA profiles.

The isolated Linux trial needed process-local `PROTON_NVIDIA_NVCUDA=1` for the
helper to verify the actual Ada device, plus early Streamline device setup.
Loading succeeded before higher native counts were usable: the helper's
asynchronous activation had to finish before feature creation. A status file or
successful DLL load alone is not readiness evidence. Confirm the selected
helper, provider and runtime in the actual game, then require current Streamline
capability/state and advancing generated presentations for the requested count.
No private experimental capability override belongs in the game path.

The isolated helper trials used synthetic owned images and do not qualify this
proxy route, a newer runtime pair, game output quality or presentation pacing.
Exercise the exact complete candidate and each offered count, return to Off,
then check pause/resume, travel, output changes and normal shutdown. Unsupported
counts must remain unavailable with an accurate status rather than silently
claiming a smaller multiplier. Keep the native path separate from the required
software/mixed presentation integration, which compute proof does not implement.

## Complete game bridge candidate

The dev.2 candidate adds `mcd2-mfg-game-bridge.dll`, built by
`experiments/providers/build_mfg_game_bridge.py`. Its plain C ABI keeps the
Microsoft C++ SDK calls inside the bridge. Native CI runs a dynamic CPU-only ABI
fixture; it does not run the GPU host or acquire DLSS Enabler.

The installer acquires the exact external 4.9.0 module separately, under the
author's terms, into `MCD2Graphics/mfg/dlss-enabler-headless.dll`. It is loaded
through its headless interface, rather than installed as a proxy. Its hash is
checked before loading. The owned bridge and its receipt belong to the mod
payload; the external runtime does not.

The candidate offers separate DLSS and FSR generated-frame counts. One rendered
frame plus the two counts is capped at 6x. Actual device capabilities admit the
exact split and native interpolation grid; 2 DLSS + 3 FSR uses native positions
2/6 and 4/6 plus FSR positions 1/6, 3/6, 5/6. Native Streamline retains its independent
maximum. The extra page has no strategy selector; ordinary Video FG defaults to
one generated frame (2x). These controls and the Busy-sample diagnostic correction
supersede dev.2 and require a fresh complete-package game replay.

The custom presenter owns one final swapchain timeline and a virtual application
buffer ring. A single bounded worker schedules generated phases against measured
real arrivals and the actual output refresh, with backpressure rather than an
unpaced queue burst. Software/mixed multipliers can lower the real rendering
rate to fit that output cadence; they do not rewrite the game's stored FPS cap.
The Diagnostics page distinguishes this scheduled limit from measured FPS.
Incoming VSync Off remains Off. Unknown refresh reports unavailable timing and
presents real frames. Displayed cadence and VRR still require game qualification.
It publishes only complete ordered batches, keeps generated images
out of real history, binds source/session/world/epoch, and retains resources until
the actual queue fence completes. Off disables generation immediately; an
engine-confirmed resize switches back to native buffers and presentation without
its frame-copy work. Enabling the custom route again requires another confirmed
resize or a restart. If an application has obtained a virtual waitable handle,
live native switching requires a restart, while normal resize remains available
on the virtual real-only path. Failed resize retains the previous valid route.
A missed scheduling window presents only the real frame and reports that
fallback; it does not publish a late burst or claim a generated batch.

Software mode uses the existing hardware-supported low-latency path. Anti-Lag 2
requires the actual AMD driver interface; it is not offered merely because FSR
is selected. The custom presenter shares the persistent Anti-Lag context and
marks each generated/real presentation. Diagnostics count actual successful
component presentations, independently of saved mode or compute success.

The owned host checks actual-source presentation and lifecycle outside MCD2.
These checks cannot establish game image quality, pacing, menu operation or a
clean game exit. Full Linux game qualification remains next; Windows/Radeon
startup follows with MFG Off. Windows MFG, Game Pass and Xodus remain separate
runtime gates. HDR format calculations alone do not prove HDR display output:
the isolated desktop rejected PQ colour-space activation. Preserve this failed
control and require the actual game's HDR check before claiming support.

Performance benchmarks remain on hold.
