# Consolidated menu transaction

`src/providers/menu_requests.hpp` implements the portable transaction processor
for the next settings cutover. `menu_save_transport.hpp` now connects it to an
opt-in native settings worker and a separately built NeoRune qualification
client. The released menu and its legacy slots are unchanged; this client sends
preference-identical requests only, not real control edits.

## Ownership

The menu sends one complete requested-settings snapshot. One native settings
worker owns one `MenuRequestWriter`, bound to its store and session. The menu
does not publish the committed record itself. Native initialization/migration
must establish that record before the writer accepts menu requests; missing or
corrupt records are never silently recreated by this processor.

The request includes all SR provider preferences, FG provider/strategy/multiplier,
latency provider/mode and HDR calibration. Future or unavailable providers can
remain requested preferences. Acceptance proves persistence only; capability
resolution, effective-state receipts and safe renderer activation are separate.

## Transport payload

All words are unsigned 32-bit little endian. Total length is 168 bytes.
This describes the payload, not an installed transport or C++ structure dump.

| Offset | Value |
| --- | --- |
| 0–7 | `MCD2RQ1` followed by NUL |
| 8 | Envelope version 1 |
| 12 | Length 168 |
| 16 | CRC-32/ISO-HDLC of the whole envelope, with this word zeroed |
| 20 | Native session, 1 through INT32_MAX |
| 24 | Menu sequence, 1 through INT32_MAX |
| 28 | Expected committed settings revision |
| 32 | Expected committed settings checksum |
| 36 | Reserved zero |
| 40–167 | Existing 128-byte graphics-intent record, including its own checksum |

The desired revision must be exactly the expected revision plus one. Migration
provenance must match the existing committed record. A session's sequence cannot
be reused with another payload. CRCs detect accidental corruption; they are not
authentication, capability checks or permission to initialize an SDK.

## Commit and retry rules

- Decode fully before any store access; failed decoding leaves outputs unchanged.
- Reject wrong sessions, older sequences and changed payloads under the same ID.
- Re-read the authority and compare revision **and** checksum. Check all transport
  bytes again immediately before committing, then use the store's locked CAS.
- Preserve provenance; do not turn current legacy/runtime ACKs into new provenance.
- A preference-identical request is acknowledged without increasing the settings
  revision or writing another file.
- An identical retry after a lost ACK does not republish. Its receipt remains
  valid only while the actual committed record still matches; otherwise report
  `Superseded` and reload.
- Only `Busy` allows the same request to retry publication. Conflict, changed
  source, corrupt/missing authority and other errors require the caller to reload
  or perform explicit initialization/recovery. Never edit an existing request ID.
- After `PublishedSyncUncertain`, preserve the uncertain receipt and reload the
  visible record. Never claim rollback or blindly repeat the old CAS.

A receipt is a typed commit result keyed to the session and menu sequence, with
the resulting record stamp and store status. It must not be shown as proof that
FSR, DLSS, FG, HDR or low latency is active. Effective-state acknowledgements must
separately match the committed revision and current renderer session.

## Evidence and next integration

Local GCC execution passes 1,512 damaged/truncated envelope cases and semantic
checks covering AMD/mixed requested providers, independent preset preservation,
session/order/reuse, lost ACKs, superseded commits, no-op requests, migration
provenance, store failures, uncertainty and Busy retry. A real temporary store
tests publication, a competing commit during the final source recheck, and
corrupt-record preservation. MinGW compilation also passes. Both CI platforms
run these fixtures; they use synthetic data and do not open game installations.

Next, implement the consolidated NeoRune transport and authority snapshot
response. Freeze legacy **preference** writes only after successful migration;
keep runtime context separate. Wire one native worker as the sole publisher,
move runtime readers to the committed authority and qualify real menu edits,
restart selection, fallback, presets, HDR and latency across platforms. Until
then this transaction processor is a tested component, not a completed cutover.

## Experimental UE save transport

The request slot is `MCD2GraphicsProviderRequest.sav`, class
`ProviderRequestSave`, with IntProperty fields `W0` through `W41`. Each signed
integer preserves one complete little-endian wire word, including CRC values
with bit 31 set. Default-zero fields may be omitted by UE serialization. Unknown,
duplicate, noncanonical or out-of-range field names, unexpected classes/types,
array indices, flags, sizes, malformed strings and trailing bytes are refused.
The existing nonnegative legacy parser is unchanged.

The response slot is `MCD2GraphicsProviderAuthority.sav`, class
`ProviderAuthoritySave`, fields `W0` through `W43`. Its 176-byte envelope is:

| Word | Meaning |
| --- | --- |
| 0–1 | `MCD2AS1` followed by NUL |
| 2–4 | Version 1 / length 176 / envelope CRC (word 4 zeroed) |
| 5 | Current native worker session |
| 6–7 | Last matching-session request sequence / commit status |
| 8 | Authority load status: Ok / Missing / Invalid / IoError |
| 9–10 | Receipt's committed revision / checksum |
| 11 | Receipt's store result |
| 12–43 | Current canonical graphics record, or all zero when unavailable |

A response can carry a historical receipt beside newer current settings. Match
both the request ID and receipt/current revision **and checksum** before treating
it as the current saved selection. No response means feature activation, SDK
eligibility, effective settings, runtime context or successful rendering.

The worker writes `Saved/MCD2Graphics/ProviderSettings/intent-v5.bin`. While this
record is missing, initialization requires stable validated legacy snapshots and
a complete final byte recheck. Initialization-only observation refuses to update
a record that appeared between reads, and commits with an empty expected stamp
so a competing publisher during the recheck wins without being overwritten. Once present, the legacy observer does not update
it. Corrupt records remain corrupt. Wrong-session requests cannot replace the
current session's acknowledgement. Responses require a verified UE-created seed;
the worker retains its engine header and publishes only its own bounded fields,
with a persistent response lock, flushed pending file and atomic replacement.
Identical responses are not rewritten at each poll.

Set `[Providers] ConsolidatedMenuTransport=1` in the existing bootstrap policy
only for this experimental transport trial; default zero performs no transport
IO. It runs on the existing settings worker independently of NVIDIA availability
and does not move SDK work onto a rendering callback.

`experiments/providers/build_menu_transport_probe.py` builds a separate,
source-recorded UI trial with the pinned NeoRune tools. It makes a guarded private
actor overlay and includes `neorune_transport_probe.cs`; it does not edit the
released actor, install files, download dependencies or package a release. The
existing actor drives the polling timer. The no-op client copies every provider
preference and migration field, advances only the desired transaction revision,
and computes both checksums using NeoRune-supported integer operations.

See [the live transport trial](MENU_TRANSPORT_2026-10-08.md). The remaining cutover
must wire actual controls, freeze legacy preference writes after migration,
separate runtime context/ACK transport, and move bootstrap/SR/FG/display readers
to the same committed record. This trial does not implement those steps or FSR.
