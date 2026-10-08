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

**Not yet admitted:** the normal Video rows still use the existing NVIDIA path.
The new client and snapshot must be connected to the continuous FSR controller,
its current-game source acknowledgement and automatic source rollback before
FSR is exposed there. Compilation and transport checks do not qualify active
AMD hardware or FSR frame generation. Published installer payloads are unchanged.
