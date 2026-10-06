# Transparent package and release gate

The guided installer now uses self-contained folder deployment with a visible
`payload/`. Extract the full outer ZIP and run the installer in that folder.
There is no embedded payload ZIP, native runtime self-extraction, updater or
background dependency downloader. The user still selects official dependency
downloads; pinned hashes, ownership, process refusal and rollback remain in force.

Build receipts identify the deployed files and explicitly leave signing and
Windows qualification false. A cross-compiled Windows installer is a build
artifact, not a qualified release. Test fresh install, repair and uninstall on
Windows using the complete folder. The packaging candidate still contains the
pinned rc.1 payload; it does not install the experimental FG trial.

Before a new public release:

1. Freeze source and exact owned payload. Set correct version/product metadata
   for every owned Windows PE, including native addons and bridge libraries.
2. Sign every owned PE with a trusted, stable publisher identity and SHA-256
   Authenticode; apply an RFC 3161 SHA-256 timestamp. Verify the signatures.
   A self-signed certificate is not a substitute. Signing is not configured by
   this change and remains required.
3. Recompute all manifests/receipts after signing and qualify those exact bytes.
4. Scan every final owned PE and the final ordinary ZIP. Record the results,
   exact hashes and source/CI provenance. Do not rebuild or repack afterwards.
5. Submit the functional final candidate for Nexus clearance before promotion.
   If quarantined, retain the file and send a compact source/build/signing/scan
   review packet. GitHub remains the documented download fallback.

The outer release ZIP must have no nested ZIP/7z/RAR and no packed or encrypted
executables. The builder rejects nested archives in its output and nonempty
output directories. Keep all required .NET/Avalonia files visible beside the
installer. Vendor graphics runtimes remain outside the owned payload.

This layout improves inspection; it does not guarantee clearance. See
[Nexus quarantine guidance](https://help.nexusmods.com/article/117-why-has-my-mod-been-quarantined)
and [Microsoft folder-deployment documentation](https://learn.microsoft.com/en-us/dotnet/core/deploying/).
