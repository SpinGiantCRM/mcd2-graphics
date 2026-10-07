# Windows MSVC framework mitigation — 7 October 2026

The resumed Windows/AMD comparison reproduced the shutdown access violation
with the original framework again. The same-source MSVC A replacement exited
cleanly in the matched full-package comparison and in the installation produced
by the candidate's shipped installer core. The candidate dependency pin now
selects this exact tested build. This is a bounded mitigation; the precise
ReShade/game/Windows shutdown defect remains unidentified.

The original [public installer qualification](WINDOWS_INSTALLER_QUALIFICATION_2026-10-07.md)
remains **FAIL** for its frozen inputs. The new candidate remains **HELD / full
qualification incomplete**, rather than inheriting that package's qualification.
The earlier investigation stop is historical; the maintainer subsequently
requested continued testing and a fix. No release, merge or Nexus update occurred.

## Changed inputs and Windows reasons

The six-row change/why/Windows-requirement table in
[Windows compatibility](WINDOWS_COMPATIBILITY.md#msvc-framework-mitigation-candidate--7-october-2026)
is the preservation contract. Specifically:

1. `dependencies.lock.json` selects MSVC A and its separately named ZIP. The
   repeated shutdown AV was also reproduced with the old framework alone;
   changing our renderer speculatively would not address that evidence.
2. The new separate archive includes the framework, license, instructions and
   pinned build receipt. Source diff, toolchain and eleven submodule revisions
   are recorded beside it. A retains PR435 and the successful-Reset lifetime
   patch; it does **not** contain the investigated descriptor-heap patch.
3. Stored ZIP members and LF attributes make the new archive byte-reproducible
   on Windows and Linux. Windows CRLF conversion and different Deflate
   implementations otherwise change hashes even when inputs look identical.
4. The dependency packager accepts explicit historical/current inputs and checks
   the extended receipt against its pin. The old compressed archive and old
   input copies stay frozen; reproducing its compressed bytes requires its
   original compressor. Rebuilds need their own hashes and validation.
5. New Python regressions check exact reproduction, provenance, historical
   preservation and changed-runtime refusal. Installer-core tests accept the
   new archive without override and refuse the old archive under the new pin.
   Both Windows and Linux CI jobs remain intact.
6. Candidate instructions and this separate validation record describe changed
   inputs and limits. No native addon, UI, shader or NVIDIA runtime was changed.
   Runtime dependency binaries remain outside mod installer payloads.

Compiler, recipe and flags changed together. The comparison does not establish
that MSVC itself fixes the fault, or that an upstream heap fix caused the result.

## Frozen candidate and host

Testing started from remote-matched PR #10 commit
`69a7bfe18ff098c985af9f6f8b8acfce1eba17d8`, then changed the dependency and
packaging inputs described above. All twelve own payload hashes are unchanged
and appear in the [sanitized record](../qualification/fg-release/windows-msvc-mitigation-2026-10-07.json).
The exact tested self-contained installer folder has a separately retained
[build receipt](../qualification/fg-release/msvc-mitigation-installer-build.json).
This local rebuild is not workflow 37554384729's frozen installer artifact.

| Input | Exact value |
| --- | --- |
| Manifest | 0.3.0-preview.1, twelve verified own files |
| Installer executable SHA-256 | `c5f37d9f40098cc312b1c3cdaef2d635bd2e6783e78e200996f663c4ed44ed9d` |
| Shipped installer core SHA-256 | `0ffa0a496315bf670896b1088aa05bcccadffdf28feef26267af39a37327c88c` |
| Framework DLL / d3d12.asi SHA-256 | `ce808bb1494415586cfb2ec5638a3ab0f0cea3879e45146a34976740b59fcf48` |
| Separate dependency ZIP SHA-256 | `610314d160a28f743cd2cfdc593c83b818bb2ead8576671d870c8f8652e95168` |
| Loader | Official 2.3, complete recognized set; ZIP `982ccc00b25a83da7fb3081739651b0e1a16d62ca990696ba64d89e8617c3374` |
| Other dependencies | Recognized RenoDX nightly-20260928, DLSS 310.9.1, Streamline 2.14.1; all overrides Off |
| Host | Windows 11 Pro 10.0.26200.9457, AMD Radeon 860M, driver 32.0.31041.1004 |
| Game | Steam 1912410, build 25647713; shipping SHA-256 `231147bd0c655a4ae73f90873675d42917f2bfb3a9ee164fc64f217d6d6bd4ef` |
| Display | 1920 × 1200, Windowed Fullscreen, SDR, VSync Off, no frame limit |

The build folder was independently checked against all 235 receipt entries,
the candidate manifest and dependency lock before installation. Its actual
shipped `MCD2.Installer.Core.dll` performed recognized-old-version upgrade,
repair, uninstall with original proxy recovery, fresh reinstall and final
uninstall, using real process guards and no dependency overrides. These are
**API checks**, not the checklist's GUI qualification. Runtime was launched
from Steam after the actual fresh reinstall; it was not manually staged.

## Runtime comparison

Every row below held the actual shipping-process handle before normal Alt+F4;
no force kill was used. Gameplay duration starts at the first observed hub
frame and ends at the close request, excluding menus, loading and shutdown.

| Run | Installation | Observed hub seconds | Close-to-exit seconds | Shipping code |
| --- | --- | ---: | ---: | --- |
| Resumed original framework, FG On / Reflex Boost / Quality | Guarded comparison staging | 318.80 | 14.51 | `0xc0000005` |
| Resumed MSVC A, same fixture | Guarded comparison staging | 317.72 | 87.83 | `0x0` |
| MSVC A, same fixture | Exact shipped-core fresh reinstall | 380.17 | 4.09 | `0x0` |

The resumed original fault matches the prior six events: access violation
reading address `0x10`, `R15 = 0x10`, fault bytes `4d 39 27` before the jump to
shipping RVA `0x8bdcb86`. Raw dumps stay private; the object/cause is unidentified.

The earlier two full-package A runs also exited 0 after 306.70 and 331.08 seconds
of observed hub play (4.11 and 4.02 seconds to exit). These are supporting
observations on unchanged payload hashes, not new installer GUI results.
The clean 87.83-second exit remains a material delay. The earlier vanilla
control also exited 0 after a 91.48-second delay; neither delay is erased.

The actual installed run survived the 13.2-second startup probe and a
69.3-second gameplay probe with all six exact addon/bootstrap/framework/bridge
hashes loaded, plus RenoDX. Visible movement worked. Current-session context
and acknowledgements agreed: Quality requested at 67%, DLSS unsupported
(`phase=3`, `error=1`); FG On requested but unavailable/inactive (`phase=2`);
Reflex Boost requested but unavailable/inactive, fault 0. Closed-game fixtures
touched only mod preference slots. No player/account save was copied or restored.

## Checklist disposition for changed inputs

The [original checklist](WINDOWS_QUALIFICATION_CHECKLIST.md) retains its frozen
input table. These are its check IDs applied to this new candidate; an API
pass does not replace a required GUI check.

| IDs | Status | Observation / remaining work |
| --- | --- | --- |
| B1 | PASS | Host, game and display recorded above. |
| B2 | PASS | Closed shipping process; hash-guarded recovery snapshot and final restoration. |
| B3 | PASS | Real intact-candidate uninstall and fresh reinstall; original proxy recovered. |
| B4 | NOT RUN | Built from a path with spaces; standalone GUI without SDK/runtime was not exercised. |
| I1 | NOT RUN | Actual core accepted Steam identity and five dependencies; GUI/wrong-folder flow not repeated. |
| I2 | NOT RUN | Actual core install passed, all own/dependency hashes and Loader 2.3 verified without override; GUI not repeated. |
| I3 | NOT RUN | Real guards retained and exercised by regression fixtures; in-game GUI refusal not repeated. |
| I4 | NOT RUN | Actual shipped-core intact repair passed; GUI not repeated. |
| I5–I6 | NOT RUN | Missing/corrupted-owned-file safety covered by core regressions; real-game GUI fixtures not repeated. |
| I7 | NOT RUN | Override safety regressions retained; five GUI acknowledgement/reopen flows not repeated. |
| I8 | PASS | Disposable core regression fixtures, exact new dependency acceptance and old dependency refusal. |
| I9–I10 | NOT RUN | Actual core uninstall/reinstall/startup/final uninstall passed; required GUI route not repeated. |
| U1 | NOT RUN | Loader 2.3 visible at main menu; current candidate Mods-page metadata not repeated in both menus. |
| U2 | NOT RUN | Main-menu Video controls used; injected gameplay shortcuts and HUD inventory click did not open the menu. |
| U3 | NOT RUN | Quality 67% exercised; full slider/preset/minimum matrix not repeated. |
| U4 | PASS | Closed-game FG On/Reflex Boost preferences returned with matching current-session acknowledgements; Quality set from main menu. |
| U5 | NOT RUN | More than five minutes of responsive hub play passed; dungeon round trip not exercised. |
| U6 | NOT AVAILABLE | No physical controller used. |
| A1 | NOT RUN | Exact loaded chain passed; all-Off baseline not repeated with new installed candidate. |
| A2 | NOT RUN | Quality unsupported fallback resolved; separate gameplay DLAA/Custom/source-size matrix not repeated. |
| A3 | PASS | Seeded mod-only FG On/Reflex Boost; both unavailable/inactive without Reflex fault or restart requirement. |
| A4–A5 | NOT RUN | Existing native-Windows pacing guards retained; dedicated sanitized event/model-hint checks not repeated. |
| N1–N5 | NOT AVAILABLE | Supported NVIDIA, active FG/Reflex, performance and HDR not available on this AMD/SDR host. |
| E1 | NOT RUN | Normal window-close runs measured; Save and quit route not reached through injected gameplay controls. |
| E2 | PASS | Actual process exits measured within 120 seconds; the delayed clean exit remains disclosed. |
| E3 | PASS | Final actual uninstall followed by guarded restoration of 39 baseline entries; input bindings unchanged. |
| E4 | PASS | This report and sanitized aggregate retain changed inputs, outcomes and limits. |

Local checks passed: 17 installer Python regressions, 118 installer-core checks,
16 bounded-FG Python regressions and 6 build-toolchain regressions. Windows and
Ubuntu CI must be checked on the pushed change. Linux gameplay runtime was not
available on this host. Original preview.1 tag/assets, NVIDIA dependencies,
player/account saves and raw diagnostics were not published or replaced.

Decision: adopt the exact MSVC dependency as a **held Windows shutdown
mitigation candidate**. Startup, hub survival, AMD fallback and normal-close
observations pass for the stated inputs. Full public installer/menu/shutdown
qualification and a causal crash repair are not established.
