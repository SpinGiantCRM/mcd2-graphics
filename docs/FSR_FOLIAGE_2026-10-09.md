# FSR foliage motion ownership — 9 October 2026

FSR now shares the material motion-vector policy already used by DLSS/DLAA.
The former UI checked only the legacy NVIDIA acknowledgement, so a valid FSR
selection never acquired this setting.

The UI requires a matching authority session, request revision/checksum, world,
observed source scale and successful FSR evaluations before setting
`r.Velocity.EnableVertexDeformation=1`. It still requires the verified
`r.VelocityOutputPass=2` path. A temporary Evaluate phase can retain an existing
lease but cannot acquire one. Native, authority loss and invalid runtime records
release it. External changes take precedence; restoration checks the captured
original value rather than assuming a default. NVIDIA eligibility is preserved.

## Bounded evidence

The source-matched NeoRune UI compiled and ran through the normal Video menu on
CachyOS/NVIDIA. FSR Quality produced 3,148 successful evaluations at the recorded
checkpoint, rendering 2560×1440 into 3840×2160 with no FSR error. The material
setting was acquired with successful readback, retained through the pause menu,
and restored with successful readback after selecting Native. Native source
resolution returned to 100%. FG was Off throughout this check.

The focused fixture passed 41 ownership checks. The actual C# client tests cover
current identities, source-scale tolerance, transient phases, corrupt records,
lost authority and retirement. Existing NVIDIA and external-owner cases remain.
The candidate receipt pins UI sources and its three own payload files.

Normal quit closed the window but left a stale process requiring targeted
termination. This run **does not qualify shutdown**. The original twelve mod
payloads and backed-up mod settings were restored byte for byte afterward;
added trial dependencies were removed. Player/account saves were not patched.

[The sanitized receipt](../qualification/providers/fsr-foliage-2026-10-09.json)
records exact hashes and limits. Windows/AMD runtime, longer lifecycle and
matched image/performance comparisons remain required. This is source and
candidate evidence, not a replacement public installer or a measured quality claim.
