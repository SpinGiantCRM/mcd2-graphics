# Report a graphics problem

Use the [bug report form](https://github.com/SpinGiantCRM/mcd2-graphics/issues/new?template=bug.yml). Intermittent crashes and visual glitches are useful reports even when the next attempt works. A clean retry does not invalidate your observation.

If the form does not yet have separate timing, FG/HDR or crash-message questions, put those details in **Observed behaviour** or **Additional sanitized evidence**. The guide works with the existing public form as well as the expanded candidate form.

Include the mod version or candidate commit, Steam/game build, Windows or Proton version, GPU and driver, and dependency versions. The installer’s **Support → Create diagnostic report** exports allowlisted versions and file verification results; review that text before attaching it. It does not capture a gameplay crash or prove the selected rendering mode was active.

Describe the settings at the time: Native/FSR/DLSS, preset or custom percentage, Frame Generation, HDR, output size and window mode. Mention other graphics mods and overlays if known. Say whether you were online or offline and in the starting menu, hub or a dungeon. Do not include party codes or account details.

Give the shortest sequence you remember, approximate time since launch, and whether the failure happened when switching away from the game or returning, resizing, travelling, using **Save and quit**, or closing the window. State how often it happened among attempts already made. There is no need to repeatedly provoke a crash or modify your saves.

Describe what you actually saw: a visible crash message, frozen window, black lines that persisted after returning, unexpected main-menu return, or only a Windows fault recorded while quitting. Include whether you could resume play or quit normally, and how long exit took. These observations can have different causes.

For a Windows fault, optional useful fields from **Event Viewer → Windows Logs → Application** are the event time, exception code and faulting module for the game. For Unreal’s crash dialog, copy only the error text, such as “GPU Crash dump Triggered”; remove LoginId and other account identifiers. An exception code alone does not identify the responsible mod, framework, driver or game component.

Keep raw Unreal crash folders, memory dumps and full logs private. They may contain account information, personal paths, process memory or proprietary game data. Do not attach executables, runtime DLLs, saves, tokens or entire folders to a public issue. Start with the sanitized diagnostic report, a short sanitized error excerpt and a cropped image if useful. A maintainer can request a specific additional capture and private transfer route if needed.

## Current Windows candidate observations

The dev.25 Windows/Radeon testing recorded an intermittent FSR Quality / FG Off focus-loss GPU crash, a Native black-line report, and intermittent shutdown access violations. Later focus-switch runs were clean. Shutdown faults also occurred in a reduced ReShade + Blueprint Loader configuration without this mod’s graphics payload. These are observations, not established causal attribution or a claim that all crashes share one cause.

If your report resembles one of these, submit it to this project first with the exact inputs and trigger. We can compare the evidence and route a demonstrated dependency defect upstream. You do not need to diagnose the owner or create accounts with several dependency projects before reporting here. Do not disable Windows protection, alter driver timeout settings or patch the game to gather a report.
