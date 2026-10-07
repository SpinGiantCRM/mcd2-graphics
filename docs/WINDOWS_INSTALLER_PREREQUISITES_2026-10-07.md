# Pinned Windows installer qualification attempt — 7 October 2026

**BLOCKED: missing pinned framework.** This attempt followed
[the qualification checklist](WINDOWS_QUALIFICATION_CHECKLIST.md) after pulling
PR10 head `4d875e105ff1a46f1e028a7d963f57f8c716d912`. No installer or game was
launched, and no install, startup, Loader 2.3 gameplay or AMD fallback pass is
claimed. The earlier developer-trial results do not qualify this package.

## Verified inputs and independent checks

- Pinned installer source: `38c348b953641946483fc28ec5722eb10e5dbd60`;
  [workflow 37554384729](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37554384729), artifact `11454227703`.
- Downloaded artifact ZIP SHA-256:
  `397363ffb383a45dc1230f927542168c7a890ca5277b9955038b25190304db48`,
  matching the checklist and GitHub artifact digest; ZIP integrity passed.
- Installer executable SHA-256:
  `12485112d875a8c5f3472204fe2651cc8e9be14a770edf47af28cab6a73086eb`.
- All 235 Windows receipt files and all 12 expanded own-payload files matched.
  Manifest version is `0.3.0-preview.1`. Manifest and dependency lock matched
  the pinned source's git bytes independently of the build receipt.
- Official Loader 2.3 ZIP SHA-256:
  `982ccc00b25a83da7fb3081739651b0e1a16d62ca990696ba64d89e8617c3374`;
  all three archive containers matched the recognized 2.3 release set.
- Local Windows checks passed: 13 installer Python tests, six build-recipe
  tests, 14 FG Python tests, 116 installer-core checks, and the statically
  compiled Wine Reflex pacing policy executable. The disposable core fixtures
  establish I8, not public GUI or game runtime qualification.
- All four portable/Windows/GUI jobs passed for both
  [pinned source CI](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37554384574)
  and [pulled head CI](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37555758248).
  Both platforms remain enabled.

## Missing input and preservation requirement

The checklist requires the separately supplied
`ReShade-PR435-4eb9056-reset-epoch-x64.zip`, archive SHA-256
`38b1affa05445a0801372f1f17be76f87e0ab5777c769a09031cd4e23dee7cb2`,
containing framework binary SHA-256
`16c05f65b47b1a1f7b21eaa9a89cfbb7868b72894aab58610763c0e4014656b1`.
It was absent from the searched local project/download locations, GitHub releases,
workflow artifact inventory and PR10 body/comments. The maintainer did not know
its location. The repository has a deterministic packaging recipe, but this
host lacks the exact pinned runtime and sanitized receipt required by it.

This is a distribution prerequisite, not a demonstrated Windows rendering
failure. The Windows developer build has a different framework hash; replacing
the required dependency or relaxing its hash would invalidate the exact-package
test. No such substitution, override or speculative rendering fix was made.
Resume from the input gate when the exact archive is supplied. Keep it separate
from the mod installer, with its license and build provenance; NVIDIA runtimes
remain external. Frozen preview.1 releases are unchanged.

## Baseline and full checklist record

Windows 11 Pro `10.0.26200.9457`, Radeon 860M driver `32.0.31041.1004`,
Steam build `25647713`; shipping executable SHA-256
`231147bd0c655a4ae73f90873675d42917f2bfb3a9ee164fc64f217d6d6bd4ef`.
The closed-game guard passed. Recovery copies of 16 existing game/mod/dependency
files and seven mod/video preference files remain private. All 23 still match;
tracked absent files remain absent. No game files or preferences changed,
no character/account saves were read, and no restoration was required.

The [sanitized aggregate](../qualification/fg-release/windows-installer-prerequisite-checks-2026-10-07.json)
records every B/I/U/A/N/E check ID. B2, I8, E3 and E4 passed; remaining installer,
menu, AMD runtime and exit checks were not run. NVIDIA N1–N4 are unavailable on
this AMD laptop; HDR/controller checks were not performed. Current output/HDR,
VSync and frame limit were not observed in game. No normal exit was measured.

The known ModInfo metadata gap, historical shutdown crash/delay and render-thread
hang remain unresolved. Release remains held. This change adds only the report,
aggregate and compatibility pointer so future work cannot mistake prerequisite
verification for an exact public-installer runtime pass.
