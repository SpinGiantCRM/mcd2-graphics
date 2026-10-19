Runtime diagnostics use the current game context independently of SR activity.
Native has zero SR provider/evaluation activity; it can still provide a valid
world for independently active frame generation or Anti-Lag. The context must
match the current renderer session and full authority stamp, be ready, and have
an advancing sequence observed within three seconds. SR counters retain their
own revision/world admission. Anti-Lag counters require its actually applied
revision and checksum, rather than the latest requested revision alone.

FSR activity reuses the actual native SR runtime snapshot already decoded by the
settings worker. A second try-lock diagnostics read supplies only separately
verified implementation metadata; its Busy result cannot replace activity with
zero. The worker keeps the original successful observation for at most 1500 ms,
only while the current ready session/full stamp/world still matches and the
snapshot is Active with error zero. Busy never refreshes that time. A successful
inactive/error snapshot, hard read/decoding error or identity change invalidates
it immediately. This adds no render lock, SDK operation or file access.

Each unavailable or inactive channel invalidates only its own UI activity
baseline. The first recovered eligible provider/mode sample establishes a
baseline, so 100 -> Busy/Off -> unchanged 100 cannot be reported as advancement.
Other SR, FG and latency channels can continue independently.
Native always has zero SR activity. Metadata/version availability cannot
substitute for an unavailable runtime activity sample.

The FG V1 ABI and lifetime totals remain unchanged. The additional V2 snapshot
contains counters for the exact admitted session/revision/checksum/world epoch.
AMD records that identity against the successful SDK configuration's frame ID;
the asynchronous composition callback supplies its actual SDK frame ID. Old,
unknown, duplicated, reused or evicted frame IDs cannot increment the current
epoch's diagnostic counters. A bounded 64-entry table deliberately undercounts
callbacks that arrive too late. It never relabels lifetime totals with a new
world. Its mutex is independent of the presenter gate and never spans an SDK
call. NVIDIA captures identity per presentation thread and consumes the SDK
outcome in that host call's finish-present callback. Neither diagnostic path
changes provider activation, source scale, presenter ownership or retirement.

The transport accepts only matching fresh V2 activity and retains the separately
reported implementation version as historical metadata. Missing V2 support
does not authorize activity from a V1 total. UI activity for every channel
requires the same nonzero current world. Invalid identity, expiry or opt-out
discards the baseline; the first recovered sample cannot establish progression.
Actual SDK dispatch/callback counters remain distinct from physical display FPS,
visual quality, latency reduction and active-model selection.

`tests/runtime_diagnostics_test.cpp` exercises Native-independent projection,
full authority stamps, queued previous-world callbacks, duplicate/reused/unknown
IDs, bounded retention and concurrent callback/snapshot publication. Both
portable CI platforms execute it. The exact C# client tests cover wrong/zero
world across SR, FG and latency, plus recovery and opt-out baselines, per-channel Busy/recovery, actual counter
resets, and continued FG/latency activity while SR is unavailable. These
fixtures and compilation are separate from fresh Windows/Linux game
qualification of the rebuilt complete artifact.

## Frame admission does not replace epoch identity

FG attribution selects the validated session/revision/checksum/world independently
of one frame being ready. Missing guides, a mismatched binding or a failed SDK
configuration skip that frame without erasing earlier outcomes in the same epoch.
Only admitted SDK frame IDs may contribute actual outcomes or refresh their
timestamp. Invalid/inactive requests, identity changes and explicit owner
invalidation/retirement still clear the epoch. Native active/fault fields, bounded
outcome freshness and UI advancement baselines remain required; retained totals
with active zero are historical counts. The portable regression extracts both
providers' actual binding/selection/admission decisions and checks gap recovery,
Off, changed identities, reused IDs and inactive/expired projections. This source
fix requires a fresh complete artifact and Windows runtime verification.
