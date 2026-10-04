# MCD2 Graphics roadmap

This roadmap describes current priorities, not release dates or guarantees. Items can move as implementation and testing uncover new constraints.

## Released

### 0.1.0-preview.1

- DLSS Super Resolution and DLAA.
- Native / NVIDIA DLSS selection in the game's Video menu.
- NVIDIA-style presets plus linked custom render scale.
- Output-dependent minimum render scale; 17% at 3840×2160 in the current qualified configuration.
- Safe fallback to the game's native temporal path when the DLSS path cannot be used.
- Documented RenoDX UE Extended HDR setup.
- Fresh install, uninstall and reinstall checks for the preview package.

See [`docs/VALIDATION.md`](docs/VALIDATION.md) for the exact qualified and unqualified scope.

## Priority order

### 1. NVIDIA Reflex

Investigate and integrate Reflex first. It should remain useful with Native, DLAA and DLSS SR rendering modes where supported, and it provides the latency foundation needed before Frame Generation is treated as release-ready.

### 2. Frame Generation

Investigate DLSS Frame Generation after the Reflex path is understood. Important game-specific work includes presentation lifecycle and separating the rendered scene from UI/HUD composition where required.

### 3. OptiScaler / broader provider support

After Reflex and Frame Generation, investigate complete FSR/XeSS provider support, including their supported Frame Generation paths, through OptiScaler or native integration. Choose the route with the best compatibility and maintenance cost after the provider tests.

The goal is to make the existing game integration usable with alternative providers such as FSR- and XeSS-family reconstruction where OptiScaler supports them, and to leave room for provider-specific Frame Generation / Multi Frame Generation paths where those providers and the user's hardware support them.

This work remains evidence-driven: a provider is not advertised as supported until its lifecycle and output path have actually been qualified. Provider-specific failures should stay distinguishable from MCD2 host-integration failures.

### 4. Ray Reconstruction — conditional

Ray Reconstruction follows the broader-provider work if the existing renderer exposes a genuinely useful pre-denoised lighting/reflection boundary and the material/geometry guides needed to use RR correctly.

The project will not add RR merely as a label or rebuild a large ray-traced renderer solely to claim support. If a real replaceable denoiser boundary exists, RR becomes a high-value quality feature; if not, it remains deferred.

### 5. Shadows / lighting

After the higher-priority feature work, continue investigating the game's shadow and lighting artifacts. Any replacement should preserve Minecraft Dungeons II's visual identity while improving clearly broken or unstable shadow behaviour.

## Parallel / non-blocking work

- Public ReShade/native interoperability improvements.
- Windows qualification.
- Physical-controller qualification.
- Long-session and wider lifecycle testing.
- Additional GPU / driver / resolution coverage from community reports.

## Project principles

- Preserve Minecraft Dungeons II's visual identity.
- Prefer the game's own Video menu for user-facing controls.
- Keep unsupported or unqualified features hidden rather than exposing placeholders.
- Treat Native rendering as the fallback when a modded reconstruction path is invalid or unavailable.
- Keep third-party provider/runtime failures separate from game-integration failures when evidence allows that distinction.
- Do not bundle game assets, extracted game shader binaries, authentication-workaround components or private user data.
- Do not claim support or compatibility that has not been tested or otherwise established.

## Testing wanted

The current preview still benefits from reports covering:

- Windows.
- Physical controllers.
- Long sessions.
- Other RTX GPU generations and drivers.
- Additional output resolutions and aspect ratios.
- Device recreation/removal and unusual lifecycle cases.

Use the GitHub issue forms and include the game build, mod version, platform, GPU/driver, output resolution, selected preset/render scale and reproduction steps. Remove account information, party codes, authentication data and private local paths before posting logs or screenshots.
