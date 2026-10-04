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

## Next priorities

### NVIDIA Reflex

Investigate and integrate Reflex as an independent feature before Frame Generation. It should remain useful with Native, DLAA and DLSS SR rendering modes where supported.

### Frame Generation

Investigate DLSS Frame Generation after the Reflex path is understood. Important game-specific work includes presentation lifecycle and separating the rendered scene from UI/HUD composition where required.

### Ray Reconstruction — conditional

Ray Reconstruction is a priority only if the existing renderer exposes a genuinely useful pre-denoised lighting/reflection boundary and the material/geometry guides needed to use RR correctly. The project will not add RR merely as a label or rebuild a large ray-traced renderer solely to claim support.

### Shadows / lighting

After the higher-priority NVIDIA feature work, continue investigating the game's shadow and lighting artifacts. Any replacement should preserve Minecraft Dungeons II's visual identity while improving clearly broken or unstable shadow behaviour.

## Parallel / non-blocking work

- Public ReShade/native interoperability improvements.
- OptiScaler compatibility and alternative reconstruction providers.
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
