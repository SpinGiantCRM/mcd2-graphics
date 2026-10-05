# Native HDR controls — Update 2 candidate

HDR uses the separately acquired, pinned RenoDX UE Extended addon. This is game HDR output and HDR scene processing, not an SDR conversion filter. No official HDR mastering reference is claimed.

## Normal setup

1. Install with the guided installer. It enables the required startup HDR key `r.AllowHDR=1` and temporal-AA base `r.AntiAliasingMethod=2`, recording both previous values for safe removal. These live in `Dungeons/Config/UserEngine.ini`, the project-scoped user override; the game rewrites generated Saved configuration on exit.
2. Enable HDR in your operating system/compositor and monitor. The mod cannot enable OS HDR. Linux needs a supported compositor, driver and Proton HDR path; the qualified local path uses native WineWayland.
3. Open **Settings → Video**. HDR controls appear immediately after Brightness.
4. Set **HDR Output → On**. Set **Peak Brightness** to your display's measured/specification value for its actual HDR mode. **Paper White** and **UI Brightness** default to 203 nits; adjust for viewing conditions.
5. If the help panel requests a restart, exit normally and relaunch. Native output and RenoDX calibration are acknowledged together after restart. Do not treat a saved selection as an immediate live calibration change.

When HDR is Off, calibration controls stay visible but disabled. Unsupported system HDR is explained in the help panel. The advanced RenoDX panel remains accessible through ReShade.

## Ownership and restart behavior

The native menu owns HDR output plus peak, scene-white and interface-white brightness. The display addon writes the four matching values to the selected RenoDX preset through ReShade's public configuration cache. The pinned RenoDX build reads these bindings at startup, so changes require restart. Other RenoDX settings are preserved. Use the native controls for those four values; selecting/altering presets in the advanced panel can require reapplying the native values and restarting.

The game-thread path applies native HDR output, peak and BT.2020/PQ output settings only after the current calibration revision is acknowledged without a pending restart. The installer does not broadly merge engine tweaks. Existing manual engine HDR overrides can conflict; back up and remove conflicting overrides deliberately rather than copying another player's entire configuration.

Peak calibration is display-specific. The local True Black display was tested at 410 nits; that is not a universal default. Brightness measurements are decoded output signals, not physical panel measurements. Advanced colour, LUT, sharpening and other image changes are outside these controls.

## Troubleshooting

- HDR unavailable: enable OS HDR and check the monitor/compositor/driver/Proton path, then restart.
- Restart warning persists: check RenoDX is the pinned version, its ReShade configuration is writable and the selected preset matches the native settings.
- Washed-out or clipped image: verify the display's HDR mode and peak calibration; avoid simultaneous SDR conversion or another tone mapper.
- Native and advanced values differ: apply the four values through Video and restart normally.
- Installer refuses an edited HDR startup key: your edit is retained; resolve it deliberately before repair. Uninstall restores only unchanged owned keys.

Windows HDR, supported Windows NVIDIA Reflex and AMD fallback remain required qualification gates for this candidate. See [Update 2 gate](UPDATE_2_RELEASE_GATE.md).
