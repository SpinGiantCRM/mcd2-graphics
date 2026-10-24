# Isolated MFG compute qualification

This experiment evaluates an externally acquired headless compute interface with
owned D3D12 images. It is separate from the installer and current fixed NVIDIA
MFG integration. It does not enable software or mixed-provider MFG in game.

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

Acquire it directly under the author's terms. Do not bundle, patch or install
it in the game. The runner copies only this exact external input and our own
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

The final hosted candidate from [run40079305728](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/40079305728)
was replayed after the per-call flush-response guard. All five counts passed;
all150 image measurements and saved raw images matched the reviewed controls.
The [bounded qualification record](../qualification/providers/linux-mfg-compute-2026-10-24.json)
pins the exact probe, artifact, external module and environment.

This is a short synthetic compute result, not a game, Radeon, image-quality,
latency or presentation qualification. It does not establish the internal
algorithm/provider from a DLL filename. Native NVIDIA caller-owned output and
true mixed-provider generation remain separate gates. Preserve the failed
missing-recording-flag controls alongside the corrected runs.

## Admission after this gate

Run 2x/3x/4x/5x/6x on native Windows/Radeon and Linux/NVIDIA separately. Preserve
failed runs; record OS, Proton, driver, module and binary identities. One
platform cannot qualify another. The exact external interface and provider
attribution still need confirmation before production admission.

After compute proof, connect phase outputs to one presentation owner and one
latency owner, with per-phase completion and current game identity. Then qualify
pause/resume, Off recovery, travel, resizing, HDR/SDR and shutdown. Native NVIDIA
output acquisition and mixed-provider 4x/6x have their own acceptance gate.
Performance benchmarks remain a separate later activity.
