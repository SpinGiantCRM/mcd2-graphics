# Experimental Game Pass save transport

The legacy menu uses Unreal's SaveGame slots. Steam stores these as files in
`Dungeons2/Saved/SaveGames`, which the native addons poll. WinGDK routes them to
Xbox Game Save instead, so the menu can appear while its intents never reach
the native addons and runtime availability never reaches the menu.

This opt-in ReShade API20 addon bridges only the six legacy MCD2Graphics slots.
It runs only inside the Minecraft Dungeons II package. It binds the title's
runtime through the separately acquired official `xgameruntime.thunks.dll`,
initializes its own middleware binding, selects one registered user from the package's
WGS directory, and uses the public XGameSave API on a worker thread. Ambiguous
users, unsupported schemas and blobs larger than 8192 bytes are refused.
It does not initiate sign-in or edit WGS storage files.

The three settings slots flow from Xbox to the native file cache. The three
runtime slots are seeded from existing Xbox blobs, then native updates flow back
through XGameSave transactions with readback verification. No other containers
are opened. The receipt records slot names and HRESULTs, never user identifiers
or payloads, in `Dungeons2/Saved/MCD2Graphics/gdk-save-bridge.log`.

The inspected GDK saves have 90 custom-version entries; the released native
codecs require 93. The bridge pads only the native cache's table to that size.
Before any Xbox write it checks cache-header provenance and restores the original
Xbox table, including every original GUID/version value. Padding never reaches
Unreal. Other table counts are refused, and the Steam codecs are unchanged.

Build with separately acquired ReShade 6.8.0 headers and MinGW C++20:

```text
python build_gdk_save_bridge.py --reshade-include <headers> --gdk-include <GameKit-Include> --mingw-cxx <compiler> --output <candidate-directory>
```

With the game closed, place `mcd2-gdk-save-bridge.addon64` beside the actual
`Dungeons/Binaries/WinGDK/Dungeons-WinGDK-Shipping.exe` and the existing native
mod payload. Remove this addon with the game closed to disable the transport.
Place the official x64 GDK `xgameruntime.thunks.dll` in
`MCD2Graphics/gdk/` beside the addon. Use matching GDK headers and binding.
The binding owns its initialization reference, released only after its save
handles close on ordinary addon unload; it never uninitializes the game's binding.
Keep the candidate's build receipt separate from the frozen release manifest.

Game-owned cache files retain the official WGS container modification time,
including when their payload bytes have not changed. Re-reading an old save
must not fabricate a current heartbeat. Timestamp conversion rejects missing,
negative and future values; unchanged timestamps cause no attribute write.
FG permits one additional second only when the active bridge reports a verified
one-second WGS clock. Its current-process/context checks still apply, and stale
settings still expire. Native runtime output slots keep their existing writer.

This transport does not qualify GDK rendering hooks, DLSS evaluation, frame
generation startup ownership, or the current consolidated provider channels.
Game Pass remains unsupported in the released installer until matching runtime
tests pass. The initial target is the six-slot `v0.3.0-preview.1` payload.

## Test package

The repaired preview candidate and its source snapshot are available separately
from the current-main port. See [Game Pass test installation](GAMEPASS_TESTING.md)
for downloads, external dependencies, exact paths, hash verification and restore
instructions. This bounded trial does not change the released installer's
supported-platform status.


## Shutdown candidate

The bridge claims only the actual current-process UnrealWindow swapchain.
Its non-resize destruction cooperatively stops and joins the save worker before
final DLL teardown. Store destruction closes the provider/user handles before
the middleware binding releases its own runtime reference. Ordinary addon unload
uses the same idempotent stop; final process teardown still avoids joins and SDK
calls. Auxiliary swapchains and resize do not stop the transport.

A private exit stack captured one remaining thread in xgameruntime XErrorReport
while the GDK worker had never logged stopped. This motivates earlier binding
cleanup. Candidate 312d028 subsequently passed one real-game exit on RTX 4080 /
WinGDK 1.1.2.0: SR/3x FG remained operational, the worker logged stopped with
result zero, and the shipping process disappeared from CIM/tasklist. This is a
bounded Windows pass, not cross-hardware or long-session qualification. Keep
Windows/Linux ownership checks and the terminal guards.
