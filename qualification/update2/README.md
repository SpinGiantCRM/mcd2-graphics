# Update 2 qualification inputs

This is an unpublished candidate, not an end-user release.

PR7 Windows/AMD results for the downloaded CI installer are recorded in
[the Windows report](../../docs/UPDATE_2_WINDOWS_VALIDATION_2026-10-05.md) and
`windows-checks.json`. The tested payload source is `f032d00`; the subsequent
evidence-only commit does not rebuild or qualify a different installer. Required
NVIDIA, HDR-output and input checks remain outstanding.

`own-payload.zip` contains exactly the seven project-owned files in the root manifest. SHA-256: `62b9e42f162f13423858cdfa19d3d90173666f45ba0b53c8249474f94af27116`. It contains no vendor runtimes, game content, player data, screenshots or raw logs. Existing SR/shader binaries are unchanged from preview.2.

The **Update 2 candidate installers** workflow verifies these inputs, builds standalone Linux/Windows installers, and uploads an artifact. Download that PR's workflow artifact for Windows testing. Record its commit and installer SHA-256 from installer-build.json. The local build receipts are provenance for local builds; CI output may have a different installer hash and must be qualified as downloaded.

Follow [the mandatory gate](../../docs/UPDATE_2_RELEASE_GATE.md). Windows NVIDIA Streamline execution and AMD fallback are both required, along with HDR, installer, physical-controller, lifecycle and shutdown checks. A build, CI pass or Linux result does not grant release approval. Do not replace preview.1/preview.2 tags or assets.

Use the normal Steam/Windows sign-in and the pinned official dependencies selected by the installer. Do not transfer Linux authentication workarounds or player saves. Download agreements remain the tester's responsibility.
