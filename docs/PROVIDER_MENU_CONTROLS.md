# Shared provider settings client

`src/ui/ProviderMenuClient.cs` sends complete schema-5 setting requests to the
single native settings writer. It handles SR/provider preferences, independent
FG choices, latency, HDR and reset defaults. It does not write the committed
record or infer that a saved choice is active.

Changes made while a request is pending are coalesced. Conflict handling rebases
only changed fields onto the latest authority, preserving newer user edits and
unrelated preferences. Retries are bounded. Actor travel adopts an outstanding
request unchanged rather than reusing its sequence. A late receipt can resolve
an expired wait. Invalid framing, enums, checksums and owner metadata are refused.

Named FSR dimensions remain the selected SDK's responsibility. A manual scale
remains Custom until the adapter identifies an exact named match; 100% is Native
AA, never automatically named DLAA for another provider.

When `ConsolidatedMenuTransport=1`, display/HDR and Anti-Lag read the same
committed authority. Legacy display settings cannot overwrite it. Invalid state
disables requested latency without changing HDR calibration. The settings addon
exports a 144-byte plain-C snapshot for the SR controller; snapshot reads are
nonblocking and perform no filesystem or SDK work.

Validation covers real setting changes, pending edits, conflicts, independent
provider choices, reset, travel, storage failures and malformed records. The
exact C# request also decoded and round-tripped through the native C++ decoder.
The game Blueprint compiler and the native latency build succeeded locally.
Both platform CI jobs retain their corresponding client and projection checks.

In released **0.4.0-amd.dev.25**, the normal Video rows use the shared client for
DLSS/FSR and independent FG/low-latency choices. The continuous SR controller
requires current-game source acknowledgement and automatic source rollback;
menu intent alone does not authorize reduced source resolution. Bounded
[release acceptance](DEV25_RELEASE_CANDIDATE.md) records actual Windows/Radeon
and Linux/NVIDIA checks with known issues. Earlier transport trials remain
historical inputs, not this release's qualification.

## Integration MFG controls

The dev.2 advanced Frame Generation page adds **Mode** and **Multiplier** beside
the existing DLSS preset and FSR version overrides. Basic FG On/Off and provider
selection stay in Video settings. Standard is 2x. Native MFG uses the selected
native provider's reported limit; Software MFG uses the external FSR-based
headless backend; Mixed MFG combines a NVIDIA midpoint with software phases at
4x or 6x. Software and mixed modes currently require DLSS or FSR upscaling.
Unsupported counts are skipped rather than silently reduced. Restart or the
existing confirmed presenter-switch protocol applies when ownership changes.

Diagnostics remain in their own page and optional gameplay overlay. Separate
DLSS/software component activity comes from advancing successful presentations
with the current source identity. A saved choice, version or loaded DLL is not
proof that the requested mode is running. Linux game qualification is pending;
benchmarks remain separate.
