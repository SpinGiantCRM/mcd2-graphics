# Experimental Game Pass save transport

Game Pass uses Xbox Game Save instead of Steam's local SaveGame files. The
API20 bridge mirrors exactly twenty mod-owned channels: the six legacy
settings/runtime channels, the provider request/authority and SR context/runtime
pairs, four independent advanced-setting pairs, and the diagnostics pair.
The exact classes and directions are declared in `src/gdk/save_slots.hpp`.
No player-save containers are opened.

Word channels use their existing typed CRC codecs, preserving signed IntProperty
bits and omitted zeros. Empty native-response seeds permit bootstrap but cannot
be published as active state. Named channels retain strict field allowlists and
bounds, including AMD latency and presenter-recovery fields. Session, sequence,
authority, world and replay validation remains in the existing native authority
and readers; this bridge is a transport, not another settings authority.

The bridge requires this game's package identity, the title runtime, the
separately acquired official `xgameruntime.thunks.dll`, and exactly one registered
signed-in user discoverable through the package's WGS directory. It uses public
XGameSave APIs; it does not sign in or edit WGS storage. Missing API, identity or
ambiguous-account support leaves transport unavailable. Restart after changing
accounts. Linux Game Pass through Xodus is a separate qualification target; the
launcher being able to run the game does not establish these API/storage gates.

An early identity-only entry point binds the title runtime and selects the signed-in user before FG chooses a presentation owner. It does not initialize a Game Save provider or perform cloud/blob IO; those operations stay on the worker. ReShade registers the preloaded addon through AddonInit. If identity is not ready at startup, FG stays on the native route and may require a restart.

The native cache lives under an edition-specific directory with a SHA-256 opaque
account binding, separate from Steam's settings and authority. Native consumers
wait for this bound root and never use Steam's cache for Game Pass. Logs contain
slot names and HRESULTs, not account identifiers or blob payloads.

GDK's 90-entry custom-version table is normalized to 93 entries only in the
native cache. Publication checks the original header and restores it exactly.
All blobs are bounded to 8192 bytes. Game-to-native file timestamps come from
authoritative container metadata: rereading unchanged data cannot invent a new
heartbeat, and future timestamps are refused.

A real owned game-swapchain destruction requests worker shutdown. The wait is
bounded to 250 ms. Synchronous store calls cannot be cancelled, so unfinished
work retains its worker, handles and binding; the addon is process-pinned to
keep code and return addresses valid. A completed worker closes normally outside
DllMain. This policy prevents an unbounded render-teardown join; it does not
claim to fix the game's independently reported shutdown faults.

Build with `build_gdk_save_bridge.py`, API20 headers and the pinned official GDK
headers. SDK/runtime files stay external to the repository. The build receipt
records source and GDK-header hashes. Portable codec/lifecycle checks and an
addon build are not game or Xodus runtime qualification.

## Integration candidate gates

The source candidate is `0.5.0-integration.dev.2`; the released dev.25 payload
and its qualification records remain unchanged. The guided installer requires
an independently admitted Game Pass executable SHA-256 in `game.winGDK`.
The candidate admits package version **1.1.3.0**, using the contributor-reported
full on-disk executable SHA-256
`22581b59ebf5675f706b5f27657d12b5de4c433908363800c7ac391ff586142e`
and size 193,206,272 bytes. The [identity record](../qualification/providers/gamepass-1.1.3.0-executable.json)
links the original report. Other executable hashes remain refused. An admitted
hash allows installation; it does not prove the integrated game works.

The edition-only runtime binding is acquired separately from the pinned
Microsoft GDK package. Steam installation does not acquire this dependency.
The build artifact contains the source-built bridge and its receipt, not the
Microsoft runtime binding, player saves or account data.

Before support can be advertised, qualify the same candidate on native Windows
Game Pass and separately through Xodus, including user binding, all channel
round trips, provider activation, restart and exit. The presentation wrapper
also needs native Windows Steam regression coverage. Existing contributor
results are historical inputs, not acceptance of the integrated binary.
