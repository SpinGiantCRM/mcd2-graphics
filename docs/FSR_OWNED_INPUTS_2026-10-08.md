# FSR SDK execution gate — 8 October 2026

The official analytical FSR 3.1.5 upscaler executed successfully through
CachyOS Proton SLR on the local NVIDIA adapter. This is a standalone synthetic
resource test, not an installed-game FSR implementation or Windows qualification.
No game file, game prefix, menu setting, release payload or tag changed.

## Exact inputs

- SDK v2.3.0, commit `60f4ea81909200d8542eca14dccb2628b763a9a3`.
- External `amd_fidelityfx_upscaler_dx12.dll`, SHA-256
  `d0dcccc74a43c44ba435b7a369b456e0970d8a4464e4bd683119b374f2c9fb46`.
  Its 28,761,864 bytes matched the pinned upstream Git blob
  `199de3a500a1d153b7e73fa5ca0adbd2a4ac1229` before execution.
- Probe SHA-256
  `cc7e7f2c78f08259f08e31edb64c1924c754ea903137ceb8b65ed8456a352682`.
- Probe source SHA-256
  `f6d45c68ecfff58b415ea734507bca8bc43fd36653ad32ddb4985a0bbe2d3cda`.
- CachyOS Proton `cachyos-11.0-20261005-slr`; Wine 11.0, NVIDIA RTX 4080 SUPER.
- D3D12's actual adapter LUID resolved vendor `0x10de`, device `0x2702`,
  non-software. No vendor spoof or ML-capability bypass was used.
- Plain native D3D12/DXGI through VKD3D-Proton, with no local ReShade,
  Streamline, game DXGI proxy, synthetic AMD driver or game assets.

The dynamic version query returned analytical `3.1.5` and `2.3.4`.
The test selected **the returned ID** for `3.1.5` and queried each created
context to confirm that exact provider. Version IDs are not hard-coded. No
FSR 4/ML provider was reported or requested.

## Matrix and result

SDK render-resolution queries produced these sizes for 1280×720 output.
Each preset was exercised in SDR and linear HDR, with 16 frames per context.

| FSR preset | Render size | SDR readbacks | HDR readbacks |
|---|---|---:|---:|
| Native AA | 1280×720 | 16 passed | 16 passed |
| Quality | 853×480 | 16 passed | 16 passed |
| Balanced | 752×423 | 16 passed | 16 passed |
| Performance | 640×360 | 16 passed | 16 passed |
| Ultra Performance | 426×240 | 16 passed | 16 passed |

All ten context creations, active-provider confirmations, 160 dispatches and
ten fence-ordered context destructions succeeded. Each context reset at frames
0 and 8. The SDK debug callback recorded **zero errors and zero warnings**.
The process exited with code 0; the recorded run took 14.131 seconds. This is
wall time including setup, CPU validation and fences, **not an FPS benchmark**.

Every color/depth/motion/exposure/mask/output resource is owned by the probe.
Color and output use RGBA16F; depth is R32F, motion RG16F, exposure a 1×1 R32F
texture, and reactive/composition masks R8 UNORM. Two flat color regions,
zero motion, camera parameters, SDK jitter offsets and explicit pre-exposure
are supplied. The masks are zero and sharpening is disabled.

After every dispatch, the test copies the output to its own readback buffer,
waits for its GPU fence, checks every RGB value for finite/bounded output and
checks both flat-region centers against the expected values. The output is
cleared to an invalid sentinel before dispatch, so a black or unchanged output
cannot pass. HDR includes scene-linear values up to 8; readback preserved them.
These values are **not display nits** and do not establish monitor HDR accuracy.

GPU completion precedes allocator reuse, input overwrite and destruction.
Successful destruction consumes the SDK handle; the probe explicitly clears its
pointer before another trial or its destructor. The first probe accidentally
attempted a second destruction when the SDK left that pointer non-null. That
test defect was corrected before the final matrix; it is not a game crash or
evidence that the SDK's renderer cannot execute here.

## Reproduce

Acquire the official headers and runtime from the pinned upstream commit into
an experiment directory. The build and runner verify their exact hashes; neither
downloads or packages vendor binaries. Header license notices remain in the
external SDK. See [AMD's API guide](https://gpuopen.com/manuals/fsr_sdk/getting-started/ffx-api/)
for descriptor chains, dynamic loading and version queries.

```sh
python experiments/providers/build_fsr_probe.py \
  --sdk /path/to/FidelityFX-SDK/Kits/FidelityFX \
  --windows-sysroot /path/to/private/windows-sysroot \
  --output /path/to/isolated-fsr-probe

python experiments/providers/run_fsr_probe.py \
  --proton /path/to/proton \
  --prefix /path/to/existing/isolated/compatdata \
  --steam /path/to/Steam \
  --output /path/to/isolated-fsr-probe
```

The Proton runner refuses game/Steam prefixes, changed probe/runtime bytes and
local proxy/driver DLLs. Its pass gate requires the complete preset/HDR matrix,
160 successful dispatches/readbacks, ten destructions, zero SDK diagnostic
counts and a clean process exit. A timeout fails the gate. Raw process logs stay
in the private experiment directory.

The Windows candidate CI compiles this same probe using its Microsoft ABI and
16 KiB stack guard. Its separate `isolated-fsr-probe` artifact folder contains
only our executable, build receipt and header license notice. Official runtime DLLs are excluded.
An actual Windows machine can execute `fsr-owned-inputs.exe` from a separate
directory with the pinned official upscaler DLL and inspect its JSONL/result;
hosted compilation does not qualify active AMD GPU execution.

## ReShade wrapper follow-up

The same updated probe was also run against the exact qualified PR435 MSVC
ReShade framework, supplied separately as `dxgi.dll` in the isolated experiment.
Its SHA-256 is
`ce808bb1494415586cfb2ec5638a3ab0f0cea3879e45146a34976740b59fcf48`.
The probe checked the ReShade export and the device vtable's module owner,
confirming that the SDK received a **ReShade-wrapped D3D12 device**.

Both the plain control and wrapped run passed all 160 readbacks, ten context
retirements and zero SDK errors/warnings, with clean exits. Their wall times
were 14.631 and 15.232 seconds; this setup/readback test is not a performance
comparison. The updated probe hash is
`8e99706f82d108bd5812400eb8c06b088068a05d4caacb53fb35c851f8c1d359`.
The [follow-up receipt](../qualification/providers/fsr-framework-2026-10-08.json)
records both runs separately from the original proof.

To repeat the wrapper gate, place that separately acquired framework in the
isolated probe directory as `dxgi.dll` and add `--qualified-reshade` to the
runner. The runner rejects any other framework bytes; the executable fails if
the device is not actually wrapped. Neither tool installs it into the game.
This covers the SDK/device-wrapper boundary, not the complete game bootstrap,
FG interposer chain or in-game resource lifetime.

## What remains

- Connect the existing pre-temporal game observation to provider-independent
  render-input packets and per-consumer GPU lifetime protection.
- Implement the game's FSR adapter with verified motion-vector units, jitter,
  exposure, HDR color contract and safe source-resolution acknowledgement.
- Connect its settings and effective-state feedback to the consolidated menu
  record; Native remains the fallback before an unavailable source reduction.
- Run actual game image, performance, preset-switch, level-travel and shutdown
  checks on Linux and Windows. This flat-pattern fixture does not qualify
  foliage, disocclusion, transparency, camera motion or real game history.
- Implement analytical FSR Frame Generation's presentation/guide contract and
  separately qualify provider combinations. No FSR FG or AMD low-latency benefit
  is established by this SR experiment.

The evidence removes the uncertainty about basic official FSR 3.1.5 GPU
execution in this Proton environment. It does not complete the AMD alternatives.
