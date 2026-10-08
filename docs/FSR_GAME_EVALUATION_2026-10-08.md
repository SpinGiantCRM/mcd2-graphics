# Real-game FSR evaluation gate

The analytical FSR 3.1.5 provider successfully evaluated two consecutive Native
3840×2160 gameplay inputs on CachyOS/Proton and an RTX 4080 SUPER. An NGX context
was absent during this gate. The original Native temporal pass continued to
produce the displayed image; the separate FSR result was read back privately.
This establishes actual game-input execution, not a supported FSR menu option.

## Integration

- The shared pre-temporal producer supplies current depth, backward pixel motion
  and a 1×1 exposure texture. Each consumer keeps its own recording leases.
- A plain C bridge isolates Microsoft's C++/COM ABI and the official AMD API
  descriptors from the native observer's compiler ABI.
- The bridge queries returned provider IDs, explicitly selects analytical 3.1.5,
  verifies the active provider and checks its required input bitfield. It does
  not force an unsupported ML provider.
- Context creation runs outside the observer mutex. Evaluation uses the verified
  game command/device proxy, restores the original rendering bindings and writes
  only an owned RGBA16F texture.
- The context/module remains alive until all captured command recordings are
  invalidated and the subsequent retirement fence completes. Failed retirement
  retains the generation. Readbacks are mapped only after GPU completion.
- Only an explicitly built developer probe consumes `provider-fsr-start.txt`.
  The normal addon, installer and published artifacts are unchanged.

## Measured result

Creation, both dispatches and destruction returned zero. The SDK reported zero
errors and zero warnings. Its required inputs were colour, depth, motion and
exposure (`15`); reactive and composition masks were optional (`48`) and were
not fabricated. The first dispatch reset history; the second retained it.

Both 4K output textures contained finite, nonnegative RGB values, with all
8,294,400 pixels nonblack and a sampled scene-linear maximum of 384, matching
the source maximum. Mean output values were about 0.253 versus about 0.256 for
the source. Mean absolute differences were 0.0069 and 0.0096. These values show
nonempty output in the expected signal range; they are not image-quality scores
or physical display luminance measurements.

The precise build pins and sanitized result are recorded in
[the qualification receipt](../qualification/providers/fsr-game-2026-10-08.json).
Raw images, buffers, logs, game assets, account data and vendor DLLs remain outside
the repository. All twelve original mod payload hashes and six mod-owned state
slots were restored after the game closed; experimental DLLs were removed.

## Remaining gates

This two-frame experiment does not qualify reduced-resolution reconstruction,
display substitution, native-history fallback, sustained playback, transitions,
moving-camera quality, particles/transparency or performance. The world-unit
conversion of 0.01 metres was an explicit trial assumption, not a verified game
setting. The captured reversed-Z infinite projection supplied its own near
plane and field of view. Dispatch intervals came from QPC; correct simulation
frame timing still requires qualification before enabling a supported provider.

FSR Frame Generation remains unimplemented. Active Windows/AMD execution and
Anti-Lag 2 hardware support also require their own qualification. Hosted Windows
CI compiles the bridge and both platforms exercise its source/generation guards;
CI does not substitute for those runtime checks.

Next: validate sustained FSR output and reset/lifecycle behavior, introduce the
provider through the shared settings path, qualify reduced-resolution modes,
then integrate the AMD FG swapchain and presentation lifecycle.

## Reproducing the isolated gate

Acquire the separately pinned SDK headers and a compatible Windows build
sysroot. The recipes neither download nor install vendor runtimes:

```sh
python experiments/providers/build_fsr_game_bridge.py --sdk /path/to/Kits/FidelityFX --windows-sysroot /path/to/sysroot --output /private/bridge
python experiments/providers/build_fsr_game_probe.py --reshade-include /path/to/reshade/include --ngx-include /path/to/ngx/include --output /private/game-probe
```

Use a private, reversible installation transaction and separately verify the
external runtime bytes before loading. The probe expects the bridge and runtime
under `MCD2Graphics/FSRGameProbe`. Select Native with FG Off, confirm the NGX
generation has retired, enter gameplay, then explicitly request a bounded capture.
Restore the original mod and mod-owned state files afterward. This is a developer
gate, not an ordinary installation procedure.

Reference: [AMD's FSR 3.1.5 integration guide](https://gpuopen.com/manuals/fsr_sdk/techniques/super-resolution-upscaler/).
