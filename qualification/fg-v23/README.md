# FG and DLSS foliage Windows candidate

For Windows qualification of [PR10](https://github.com/SpinGiantCRM/mcd2-graphics/pull/10). This is not a release installer.

[Download the candidate](mcd2-fg-dlss-windows-candidate-v23.zip). SHA-256:
`353c69e8c99adae7f6804463f351c0458dc8a83c31cd49f478336838b115f0d6`

The archive contains the project-owned probe, bootstrap/latency/SR binaries, clean UI, build receipts and reversible trial helpers. It includes no vendor runtimes, game assets or player data. Acquire the pinned prerequisites separately.

Follow the [install and recovery instructions](../../experiments/fg-streamline/README.md#reversible-game-trial) and [Windows checklist](../../experiments/fg-streamline/WINDOWS_FG_CLEARANCE_2026-10-06.md). Start FG Off; change the native Video-menu toggle and restart to test On.

DLSS SR and DLAA now receive the foliage vertex-deformation velocity correction even with FG Off. Native/fallback restores the owned original value. Temporary menus retain FG resources; a full release requires restart before re-enabling.

## Bounded Linux comparison

RTX 4080 SUPER, 3840×2160 HDR, DLSS Quality (2560×1440), same stationary scene; median of three short captures:

| Build / mode | Rendered FPS | SDK presentation FPS |
| --- | ---: | ---: |
| Working baseline | 124.3 | 124.3 |
| Candidate, fresh FG Off | 123.6 | 123.6 |
| Candidate, fresh FG On | 85.6 | 171.2 |

Off differed by -0.5%; On reduced rendered FPS by 31.1% and increased total SDK presentations by 37.8%. These are short scene-specific samples with animated lighting/foliage, not physical monitor cadence or latency measurements. [Exact evidence and limits](../../experiments/fg-streamline/LINUX_DLSS_FOLIAGE_FG_CANDIDATE_2026-10-06.json).

Windows/AMD can qualify unsupported-GPU fallback, menus and installer/file recovery. Supported Windows/NVIDIA hardware is required to qualify DLSS/FG. The candidate remains unqualified for release.
