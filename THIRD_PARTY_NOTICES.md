# Third-party notices

NeoRune SDK helper code incorporated into the UI package is MIT licensed; its notice is in `third-party/NeoRune-LICENSE.txt`. ReShade SDK interfaces are covered by `third-party/ReShade-LICENSE.md`. RenoDX UE Extended is an external dependency; its MIT notice is included for attribution in `third-party/RenoDX-LICENSE.txt`.

This software contains source code provided by NVIDIA Corporation. NVIDIA SDK interfaces/compiled portions are subject to `third-party/NVIDIA-SDK-LICENSE.txt`; NVIDIA SDK headers and runtime DLLs are not bundled. This project's own-source license does not apply to NVIDIA components. Users obtain the runtime from NVIDIA subject to its license.

No official endorsement by any dependency author, NVIDIA or the game's developers is claimed.

The experimental Anti-Lag 2 bridge incorporates AMD's MIT-licensed SDK header
from commit `390aa4a8c8655d0ae6e90079db2c85e103a96da3`. Its unchanged copyright
notice and full license are in `third-party/AntiLag2/`. No AMD driver or FSR
runtime DLL is included. AMD does not endorse this project.

The isolated FSR SDK probe compiles helper code from AMD's public MIT headers
at SDK commit `60f4ea81909200d8542eca14dccb2628b763a9a3`. Their copyright/license
notice is in `third-party/FSR-SDK-HEADERS-LICENSE.txt` and accompanies the probe.
The official FSR runtime remains an external dependency under AMD's own terms;
it is not included in mod archives or the probe artifact.

The self-contained guided installer includes .NET Runtime 10.0.12, Avalonia 11.3.22, SkiaSharp 2.88.9 and HarfBuzzSharp 8.3.1.1 under their upstream licenses. Their complete pinned notices are in `third-party/INSTALLER_NOTICES.txt` and embedded in the installer under Support → Third-party notices. This is distinct from the external graphics runtime dependencies, which are never bundled.

Installer IPC support includes Tmds.DBus.Protocol 0.21.3 and MicroCom.Runtime 0.11.0 (MIT); their notices are retained under third-party and in the installer’s Support page.
