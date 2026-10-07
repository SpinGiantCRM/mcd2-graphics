# Transparent package and review checks

The guided installer now uses self-contained folder deployment with a visible
`payload/`. Extract the full outer ZIP and run the installer in that folder.
There is no embedded payload ZIP, native runtime self-extraction, updater or
background dependency downloader. The user still selects official dependency
downloads; pinned hashes, ownership, process refusal and rollback remain in force.

Original build receipts identify the deployed files and leave signing and
Windows qualification false at build time. Later runtime qualification is recorded separately; build receipts are not rewritten. A cross-compiled Windows installer is a build
artifact, not a qualified release. Test fresh install, repair and uninstall on
Windows using the complete folder. The `0.3.0-preview.1` package contains
12 owned FG/SR/UI files. The patched ReShade framework is a separate, credited
dependency archive; NVIDIA runtime libraries remain external downloads.

Before a new public release:

1. Freeze source, payload and dependency hashes; complete the available Windows
   and Linux checks against the shipping files. Verify Mods-page version/help.
2. Record signing status accurately. Trusted Authenticode signing is not
   configured; these candidates are unsigned. Signing is a possible future
   improvement, not an implemented safety guarantee. Do not silently rebuild
   qualified native binaries merely to add signing or version resources.
3. If bytes change through signing or rebuilding, recompute manifests/receipts
   and repeat the affected qualification before release.
4. Scan the final owned binaries and final ordinary ZIP using available security
   tools. Record scanner/version/date, exact hashes and source/build provenance.
   Missing tools or results remain NOT RUN, not a clean scan. Do not tell users
   to disable security protection. No local scanner is configured in this run.
5. Upload only after the maintainer's release hold is lifted. Record Nexus's
   security status, retain quarantined files and request manual review when
   necessary. Await staff clearance; do not direct Nexus users to off-site copies
   of quarantined files. Keep source/build links available for staff review.
   Apply AI-Generated Content and AI Media tags where required, and hide obsolete
   benchmark cards that could misrepresent the current release.

Nexus's green badge records both internal checks and VirusTotal; blue records
internal checks without a VirusTotal result. Neither is an independent runtime
qualification. See [Nexus scan-status definitions](https://help.nexusmods.com/article/128-anti-virus-false-positives).

The outer release ZIP must have no nested ZIP/7z/RAR and no packed or encrypted
executables. The builder rejects nested archives in its output and nonempty
output directories. Keep all required .NET/Avalonia files visible beside the
installer. Vendor graphics runtimes remain outside the owned payload.

This layout improves inspection; it does not guarantee clearance. See
[Nexus quarantine guidance](https://help.nexusmods.com/article/117-why-has-my-mod-been-quarantined)
and [Microsoft folder-deployment documentation](https://learn.microsoft.com/en-us/dotnet/core/deploying/).
