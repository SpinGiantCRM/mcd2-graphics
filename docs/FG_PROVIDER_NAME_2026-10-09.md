# FG provider name serialization — 9 October 2026

A fresh developer run selected FSR Quality and AMD Frame Generation, but produced
no generated frames. The startup owner was AMD, FSR was evaluating successfully,
and the persisted authority requested AMD FG. The FG settings slot serialized
its known `FGProvider` property as `fgProvider`. The native decoder rejected that
spelling before publishing its capability/session response, leaving FG unavailable.

Unreal FName identity is case-insensitive and preserves an interned spelling.
The decoder now recognizes case variants of the current `FGProvider` property
and the old `Provider` alias. It still rejects multiple provider fields, including
different case variants or a combination of current and legacy names. Unknown
properties, invalid values, malformed framing, truncation and stale session /
authority checks remain rejected.

The synthetic fixture covers all 1,280 ASCII case variants of the two known
names and conflicting duplicates. Both CI platforms already execute this
fixture. This changes the FG menu handshake only; presentation ownership,
resource guides, frame generation and GPU capability checks remain unchanged.
The rebuilt guide observer was tested independently; its exact hashes and
limits are in the [sanitized receipt](../qualification/providers/fg-provider-name-2026-10-09.json).

## Bounded runtime regression

A normal Steam restart retained FSR Quality and AMD FG On. The serialized
`fgProvider` request produced a current-session capability and active response,
6,234 FSR evaluations and 6,200 generated frames, with zero reported SDK errors,
warnings or faults. Generation stopped in the pause menu; its count remained
unchanged between two paused checkpoints and increased again after resuming.

Save and quit to desktop removed the shipping process in 4.485 seconds without
a signal. An exit code was not captured. The original mod payloads and mod-only
preferences were restored byte for byte; added dependencies were removed.
The independent AMD FG retirement result remained `-61`, retaining its context
until process exit. This decoder correction does not repair that lifetime gap
or qualify Windows/AMD execution, image quality, performance or latency.
