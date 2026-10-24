# Anti-Lag 2 SDK provenance check

The official [AMD Anti-Lag 2 page](https://gpuopen.com/anti-lag-2/) lists the
2.0.5 Unreal plugin package. Its six engine variants contain the same DX12 API
header as this project's pinned standalone SDK, after CRLF/LF normalization.
The plugin package version therefore does not require a different native bridge
API here.

The [verification receipt](../qualification/providers/amd-antilag-sdk-recheck-2026-10-14.json)
records the official archive and normalized header hashes. No vendor plugin or
runtime was added to the mod payload. This source comparison does not establish
Radeon driver activation or a latency benefit; those require native Windows
hardware testing.
