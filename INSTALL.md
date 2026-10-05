# Installation / removal

Use a legally obtained game installation that already launches and signs in successfully. Close the game before changing files.

This preview deliberately checks the exact locally tested game/dependency files. A different version is refused; do not rename an unrelated DLL to bypass a check. Hashes and official source links are in `dependencies.lock.json`.

1. Install the **full addon support** build of ReShade 6.8.0.2155 for the game's D3D12 executable in `Dungeons/Binaries/Win64`. Obtain it from [official ReShade full-addon installer](https://reshade.me/downloads/ReShade_Setup_6.8.0_Addon.exe).
2. Install [Blueprint Loader 1.1](https://www.nexusmods.com/minecraftdungeons2/mods/2), placing its three package files in `Dungeons/Content/Paks/~mods/BlueprintLoader`.
3. Install the pinned [RenoDX UE Extended nightly-20260928 addon](https://github.com/marat569/renodx/releases/download/nightly-20260928/renodx-ue-extended.addon64) beside the game executable. The pinned binary SHA-256 is in the lock file; use this archived download rather than a changing snapshot.
4. Obtain the pinned official `nvngx_dlss.dll` from the NVIDIA repository URL in the lock file, subject to [NVIDIA's SDK terms](https://github.com/NVIDIA/DLSS/blob/374959484e79a640feaba44c93ac8cfb0a03f5b5/LICENSE.txt). The installer never downloads or accepts terms for you. This DLL is not included in the mod archive.
5. Extract this archive somewhere outside the game folder. Install Python 3 if needed. Run:

```text
python install.py install --game "PATH TO Minecraft Dungeons II" --dlss-runtime "PATH TO nvngx_dlss.dll"
```

The installer verifies the package, game executable and dependencies, refuses overwrites, and records only its six installed files. It leaves Steam options, existing configuration, dependencies and saved games intact. Earlier private trial installations must be removed first; do not load both addon filenames.

Launch once: a fresh mod profile defaults to **Native**, remembering **Quality / 67%**. Open Settings → Video and choose NVIDIA DLSS. The main menu stays Native; your saved DLSS selection activates in gameplay. Check the help panel for active/pending/fallback status. At 4K the slider cannot request less than 17%.

This preview detects unsupported rendering adapters before reducing
source resolution. On AMD, the help panel reports **DLSS unavailable; native
anti-aliasing is active.** Your DLSS preference remains saved. Close ReShade's
first-run overlay before testing game keyboard shortcuts.

## Updating from an earlier preview

Close the game. Use the earlier archive's `install.py uninstall` command first, then run this archive's install command with your existing official DLSS DLL. The installer refuses to overwrite an existing installation. ReShade, Blueprint Loader, RenoDX, game saves and mod preferences are retained. If files were installed manually, remove only the six files listed in that version's manifest; do not delete the whole game or Saved directory.

If a file was modified, uninstall retains it and reports its path. Back it up and resolve that reported conflict before installing the new version. GitHub's automatic source ZIP has no compiled payloads: download **MCD2-Graphics-0.1.0-preview.2.zip** from the release Assets.

## HDR

See [HDR setup](docs/HDR.md). HDR is a separate opt-in configuration; install alone does not enable OS HDR or change your display settings.

## Verify installed payload

```text
python install.py check --game "PATH TO Minecraft Dungeons II"
```

## Uninstall

Close the game, then run:

```text
python install.py uninstall --game "PATH TO Minecraft Dungeons II"
```

Only files recorded by this installer are removed. Modified files are retained and reported, rather than deleted. Blueprint Loader, ReShade, RenoDX UE Extended, game configuration and all save files remain. If you manually added HDR configuration, restore your own pre-HDR configuration backup separately. To reset just the mod preferences, back up and remove `MCD2GraphicsSettings.sav` and `MCD2GraphicsRuntime.sav` in the game's local `Saved/SaveGames` directory; never remove the whole directory.
