# Windows AMD qualification: updated-game dev.25 progress

**Qualification is incomplete. No exit-crash repair or release approval is claimed.**
Steam build **25910487**, shipping SHA-256
`3842102bb05fc532b22e97992d61c2248d115be80ff752ff9b6843bf4acd7005`,
is a separate baseline from the previous 25754144 results. Old address-pinned
crash probes remain retired. This is an interim record, not the final checklist.

The exact candidate is `0.4.0-amd.dev.25`, requested source
`e704a17c59e69ad4ec04191f43b0928ee0ad1097`, native artifact source
`b5719f95820acb97dcd5fcdfa60bdde291b3379a`, identical complete source tree
`775604d9599c7ae1862eaeeb878f31c611cdcc65`. Native workflow
[39774607102](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/39774607102)
and portable Windows/Linux workflow
[39774607284](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/39774607284)
passed. Artifact `12424387023` has archive SHA-256
`49ae3e0eb63992eb8d161648623d24536ad1c2f52ca0cf72eded432450ee4476`.
The freshly assembled installer SHA-256 is
`d1772ed3ce8cb8f141bd6cf9a1bf7734da11482eb58f915f3a32d37542bf9774`.

The fresh GUI install recognizes the updated executable, uses current publisher
dependency defaults with overrides off, and verifies all **18 owned / 15
dependency files**. Inputs include Loader 2.6/file 506, Streamline 2.14.16,
DLSS 310.9.16, the pinned FSR SDK 2.3.0 runtimes and RenoDX snapshot
`0.2026.1021.0108` / SHA-256
`48fa2f37d7ccfda97e3e69d5e7c1e4df272fd4aaf9978d98540416c18ce93aa4`.
The framework remains a separately selected dependency, SHA-256
`9fdac099b390ff2d88d4cf6d51685f9991ea7484e6a70fe0b092019d7adddba2`.
Loaded libraries do not establish ML-model or HDR activation.

## Completed bounded checks

| Gate | Result | Observation |
|---|---|---|
| Fresh install | PASS | Normal dev.23 uninstall, then exact dev.25 GUI install; overrides empty. |
| Intact verification | PASS | GUI verifies correct files without mutation. |
| Missing owned shader repair | PASS | Exact manifest bytes restored. |
| Modified owned shader protection | PASS | Repair and confirmed uninstall refuse without mutation; exact original restored before launch. |
| Running-game writes | PASS | Install and confirmed uninstall refuse; intact Repair performs only its read-only scan. All files and receipt unchanged. |
| Main-menu Loader metadata | PASS | Loader 2.6 and mod version dev.25, name, author and description observed. Loader-owned update-status text still clips. |
| Main-menu Video layout | PASS | FSR Quality shows numeric 67% render scale; settings/help render. Full interaction matrix remains pending. |
| Updated engine layout | PASS | Actual registry validation and marker registration accepted build 25910487. |
| FSR / FG / Anti-Lag activity | PASS | Two ordinary pre-capture/current-process intervals validate same-session/revision/world advancing counters, with no provider errors. |
| Ordinary full-candidate exit | PASS, one observation | FSR Quality / FG On / Anti-Lag On, normal Alt+F4, held exit code 0, about 7.49 seconds, no clock gap or new Application crash event. |

Machine: Windows 11 Pro build 26200, Radeon 860M, driver 32.0.32015.7029,
1920x1200 SDR output. The first strict active interval advanced SR/FG-generated/
Anti-Lag by **77/30/47**; the next ordinary process advanced **39/40/39**.
Analytical SR 3.1.5 and FG 3.1.6 were reported. These counters are not physical
display FPS, frame-time tails, image-quality or input-latency measurements.
FG correctly suspends while the game is not foreground.

## Crash investigation remains separate

An image-specific Heaps run with verified verifier modules reached the hub and
returned `C0000005` on normal close. ProcDump reported no access-violation event;
this run does not identify a faulting instruction. Its exact IFEO absence was
restored, and all installed hashes were rechecked. A later, non-verifier run
returned `1000` shortly after debugger attachment, before a close request.
Neither diagnostic result is substituted for an ordinary-release exit result.

The private capture wrapper could cancel its recorder immediately when the
target signaled exit, racing a termination dump. It now allows bounded debugger
drain first. The exact corrected wrapper passes owned clean and explicit
`ExitProcess(C0000005)` controls: one termination dump each, preserved exit
codes and completed debugger drain. This fixes capture tooling; it is not a
game-crash repair. Dumps and proprietary runtime details remain private.

Remaining checks include the complete menu/preset/advanced/diagnostics matrix,
gameplay and travel, resize/ownership restoration, override acknowledgements,
final uninstall/reinstall, and ordinary shutdown regression. NVIDIA activation
is unavailable on this Radeon-only machine. Physical controller/HDR coverage
must be reported independently. Historical shutdown failures remain disclosed;
one clean current-build exit does not establish their cause or repair.
