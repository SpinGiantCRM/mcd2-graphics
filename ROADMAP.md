# MCD2 Graphics roadmap

Planned work, in priority order. Release dates and hardware support depend on implementation and testing.

Release history is in the [changelog](docs/RELEASE_NOTES.md). See [validation](docs/VALIDATION.md) for current tested support.

## Priority order

### 1. FSR parity with DLSS

The FSR Super Resolution, Native AA, Frame Generation and compatible Radeon Anti-Lag 2 paths shipped in **0.4.0-amd.dev.25**. That release has bounded [Windows/Radeon and Linux/NVIDIA acceptance](docs/VALIDATION.md), with known issues disclosed. SR and FG are independently selectable, including DLSS SR + FSR FG and FSR SR + DLSS FG where supported. Full benchmarks remain on hold at the maintainer's request.

Prepare shared provider interfaces and frame inputs while preserving existing DLSS, FG, Reflex, HDR, saved settings and Native fallback. Qualify analytical paths first; enable ML paths only when their runtime and hardware requirements are met.

### 2. Multi Frame Generation

Deliver MFG paths for both Radeon and NVIDIA GPUs. Include single-provider software MFG on compatible older hardware and native-provider MFG where the actual device/runtime supports it. On newer hardware, retain a choice between supported native, software and mixed-provider modes; use the same admission and qualification standards for both vendors.

Add DLSS Enabler-style hybrid MFG in which two FG providers contribute different intermediate frames to one output timeline. Independent upscaler and FG choices alone do not satisfy this goal. Keep one final presentation owner and a compatible latency solution.

The fixed NVIDIA multiplier integration is one foundation for this work. It does not complete Radeon/software MFG or mixed-provider generation, which remain required milestones.

Expose only combinations with verified frame inputs, output access and stable pacing. Reserve the same interfaces for later XeSS SR, FG/MFG and XeLL support.

### 3. Ray Reconstruction and further technology

Investigate DLSS Ray Reconstruction, FSR Ray Regeneration and independent denoisers after FSR and MFG. Proceed only where the game provides usable noisy lighting signals, matching guides and a replaceable rendering stage.

Keep joint denoising/reconstruction distinct from independent denoisers. Investigate late camera reprojection separately when actual camera inputs and presentation compatibility can be established.

### 4. Shadows / lighting

Investigate remaining shadow shimmer and lighting instability while preserving the game's visual identity.

## Parallel work

- Microsoft Store / Xbox PC version qualification and compatibility.
- Wider Windows and Linux qualification, including active NVIDIA paths.
- Physical-controller, long-session and lifecycle testing.
- Additional GPU, driver, resolution and aspect-ratio coverage.
- ReShade/native interoperability and dependency maintenance.

## Project principles

- Preserve Minecraft Dungeons II's visual identity.
- Use the game's own settings menu for controls.
- Preserve Native rendering as a safe fallback.
- Keep unavailable features hidden and distinguish saved choices from active support.
- Measure rendered FPS, generated frames, presentation pacing and latency separately.
- Publish only support claims backed by evidence.
- Keep game assets, extracted shaders, authentication workarounds and private user data out of releases.

## Testing wanted

Reports from Microsoft Store / Xbox PC users, additional hardware and physical controllers are welcome. Include your store, game build, mod version, platform, GPU/driver, selected settings and reproduction steps through the GitHub issue forms.

Redact personal details. Do not upload game executables, saves or authentication data.
