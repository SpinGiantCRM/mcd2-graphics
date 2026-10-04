# HDR setup and limits

This project uses RenoDX **UE Extended** as an external HDR addon. The DLSS renderer works on linear HDR scene colour before the game's output processing; this is not Gamescope inverse tone mapping.

Back up your configuration before editing. A validated HDR output path is required first. Enable OS HDR and verify the monitor reports HDR. On Linux this depends on the compositor, driver and Gamescope setup; this release does not package platform authentication or launch workarounds.

The local tested engine settings under `[SystemSettings]` in `Saved/Config/Windows/Engine.ini` were:

```ini
r.AllowHDR=1
r.HDR.EnableHDROutput=1
r.HDR.Display.OutputDevice=3
r.HDR.Display.ColorGamut=2
r.HDR.UI.CompositeMode=1
```

The local `GameUserSettings.ini` profile also had `bUseHDRDisplayOutput=True`. Do not copy a fixed monitor brightness to another display. Set the game's HDR output nits and RenoDX peak nits for your display's actual HDR mode. In RenoDX, Game/UI brightness 203 nits was the neutral working starting point; sharpening remained off. Calibrate peak clipping and comfortable UI brightness before changing creative colour controls.

These settings are documented, not silently merged by the installer. Availability outside the tested configuration remains unqualified. The earlier captures demonstrated peak-control response and shader compatibility; they did not establish an official HDR grade. The SR mod does not implement a new tone mapper, shadow renderer, frame generation or ray reconstruction.
