# Development after 0.1.0-preview.1

## Frozen reference

The public tag `v0.1.0-preview.1` points to `0e77429cf47eea672056986f8550ef3073937446`.
The ZIP SHA-256 is `45e870c282687a4aaa9486d2cd7be26e1d237385b31b7aeaf8ae09e3ec866a19`.
Keep its source, tag and assets unchanged. New development belongs on a separate branch. A concrete release-breaking defect requires an explicit disposition; do not silently replace published files. Cut preview.2 only for concrete fixes.

## Release maintenance

Monitor GitHub issues and Nexus bugs/posts every six hours. Keep a local sanitized triage ledger; notify on meaningful changes. Windows, physical controller, device failures and long sessions are qualification work, not assumed passes. The initial GitHub issue and Nexus bug check on 4 October found no reports.

## Reflex, independent of frame generation

1. Inventory shipped NVAPI/Streamline calls and markers.
2. Verify the runtime device supports the low-latency interfaces.
3. Locate pre-input simulation start/end, render submission start/end and present start/end. Keep a shared engine-frame ID across threads. Actor Tick, command-list recording, and present callbacks are candidate observations; they are not interchangeable frame boundaries.
4. Put driver sleep before the real simulation/input boundary. Keep markers consistent in Off, On and On + Boost. Detect existing ownership before changing pacing.
5. Implement the independent provider, reset/disable/device teardown, then expose NVIDIA Reflex in the existing native Video menu only when the path is functional. Use a separate settings/runtime bridge so Reflex preferences do not churn SR feature generations.
6. Verify marker ordering, support/fallback, mode changes, latency reports and throughput. Do not promise click-to-photon improvement without an appropriate measurement.

## Broader providers

Priority after Reflex and FG: complete FSR/XeSS support, including supported FG paths, through OptiScaler or native integration. Qualify provider creation, owned-input evaluation, lifecycle, fallback and UI semantics before replacing any private SR adapter. No alternative provider is advertised as supported yet.

## FG preparation

Keep the released SR renderer unchanged. Inspect final world colour before UI composition, UI colour/alpha, and command-list/queue/swapchain lifecycle. Saved captures identify a PQ scene plus BGRA8 UI at compositor `0x378DF900`; preserve this as a lead rather than a completed lifetime contract. Check validity/content through Present, frame identity, exposure and colour conventions. Do not install an FG runtime or replace the swapchain before these gates.

## Lightweight RR gate

Inventory active denoiser inputs from existing captures first. Probe-space traces, shadow visibility and already filtered/composed colour do not automatically constitute RR inputs. RR follows broader-provider support, ahead of shadows, only if a replaceable noisy-radiance boundary and correctly decoded material/depth/motion/specular guides exist. Otherwise park RR and continue targeted lighting/shadow work.

## Official references

- [NVIDIA Reflex integration](https://github.com/NVIDIA-RTX/REFLEX)
- [Streamline Reflex guide](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideReflex.md)
- [FG integration](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_G.md)
- [RR integration](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md)
- [dxvk-nvapi low-latency implementation](https://github.com/jp7677/dxvk-nvapi/blob/master/src/nvapi_d3d.cpp)

## Current evidence — 4 October 2026

- Nine mock coordinator checks pass, including overlapping simulation/render frames and teardown during driver sleep.
- An isolated D3D12 provider probe completes 60 frames in each mode: Off, On, On + Boost. All NVAPI calls succeed. On and On + Boost return 64 complete simulation/present reports; Off returns none on this stack. Clean shutdown exits successfully. This tests driver plumbing, not MCD2 integration or end-to-end latency benefit.
- MCD2 observations expose NVAPI support but no populated simulation/present reports in the sampled title/menu/gameplay paths. No verified engine-level pre-input boundary or cross-thread frame-ID propagation hook exists yet. Reflex remains hidden in the game menu.
- A separate read-only observer identifies full-resolution PQ world colour and BGRA8 UI at compositor `0x378DF900`, with that pass targeting the current backbuffer. Alpha meaning, resource validity through Present and synchronization remain unqualified. The temporary observer is removed after sampling.
- Existing saved captures expose full-resolution reflection radiance before temporal denoising. The companion scalar is not verified as world-space hit distance, and primary noisy-colour/material guides are incomplete. RR is reconnaissance only.
- These experiments do not modify the released SR binary or UI package. Windows, physical-controller and long-session qualification remain outstanding.
