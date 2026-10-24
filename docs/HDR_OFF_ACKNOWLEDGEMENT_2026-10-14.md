# Native HDR Off without RenoDX

The dev.10 SDR setup exposed a real acknowledgement error. With RenoDX removed,
the mod's saved HDR preference was Off, but the native game setting remained On
and AMD FG output metadata still reported PQ/HDR. The latency addon returned
HDRRevision 0 / restart required whenever RenoDX was absent. The UI therefore
could not call the game's normal HDR output setter, including to disable HDR.

The correction acknowledges a valid **Off** revision without RenoDX. It makes
no calibration writes and wakes the settings worker once per changed revision.
Missing-addon **On** remains blocked. With RenoDX loaded, the existing matching
calibration, dirty-cache and restart requirements remain unchanged. No live
calibration acknowledgement, renderer bypass or ABI change is introduced.

The [fixture receipt](../qualification/providers/hdr-off-acknowledgement-2026-10-14.json)
records 13 cases executing the actual source function with synthetic addon and
configuration state. The old source compiles but fails the missing-addon Off
case (executable exit 2); corrected source exits 0. Cases cover invalid/repeated
intent, changed Off revisions, missing-addon On refusal, matching loaded HDR
and SDR calibration, dirty-cache retention and restart requirements. The check
is required in both portable CI jobs.

This is source-level proof, not a game or Windows/Radeon pass. A new complete
artifact must verify the native HDR flag and actual swapchain colour space with
RenoDX absent, then exercise FSR/AMD FG in SDR and recheck HDR with RenoDX loaded.
Preserve the dev.10 benchmark's rejected SDR samples and fatal diagnostic exit;
this output-state correction does not explain or clear that exit failure.

The candidate selector is marked stale. Installer CI must skip assembly while
the selected artifact needs a rebuild, rather than package old binaries under
new qualification expectations. Published files and tags are unchanged.
