# Contributing to MCD2 Graphics

Thanks for helping improve MCD2 Graphics. This project is still experimental, so reproducible evidence and clear scope are more valuable than broad claims.

## Before contributing

Read:

- [`README.md`](README.md)
- [`INSTALL.md`](INSTALL.md)
- [`docs/BUILD.md`](docs/BUILD.md)
- [`docs/VALIDATION.md`](docs/VALIDATION.md)
- [`ROADMAP.md`](ROADMAP.md)

The current source tree separates native code, shaders and UI under `src/native`, `src/shaders` and `src/ui`. Root-level Python tooling handles build/install/package-related tasks. Follow the existing structure unless there is a concrete reason to change it.

## Project scope

Contributions are welcome for areas including:

- rendering integration and lifecycle correctness;
- DLSS/NGX, Reflex and later Frame Generation work;
- UI and settings integration;
- HDR/RenoDX compatibility;
- provider/interoperability work;
- tests, install tooling and validation;
- documentation and hardware/platform qualification.

Please avoid combining unrelated renderer, UI and packaging changes into one large pull request when they can be reviewed independently.

## Evidence standard

For renderer or compatibility claims, state exactly what was tested.

Prefer language such as:

- **Proven / passed** — controlled local evidence for the stated scope.
- **Supported by evidence** — multiple observations agree, but coverage is incomplete.
- **Unqualified / untested** — no adequate test has been completed.
- **Blocked** — a required interface, resource or safe implementation route is missing.

Do not turn a successful feature creation call into a claim that image output, cleanup, lifecycle or performance is correct. Do not report isolated kernel cost as whole-game performance.

Useful reports include:

- game build;
- MCD2 Graphics version or commit;
- platform (Windows / Proton and relevant version);
- GPU and driver;
- output resolution and aspect ratio;
- selected reconstruction mode, preset and render scale;
- ReShade and relevant dependency versions;
- exact reproduction steps;
- whether the failure recovers to Native.

## Visual target

The project aims to preserve Minecraft Dungeons II's visual identity.

Renderer changes may use different algorithms or redistribute GPU cost, but should not arbitrarily recolour, relight or rematerial the game. For effects that are visibly broken or geometrically incoherent, especially shadows/contact effects, preserving the intended lighting relationship matters more than reproducing broken pixels exactly.

## Safety and redistribution boundaries

Do not commit or attach:

- game assets;
- extracted proprietary game shader binaries;
- account identifiers, party codes or authentication material;
- private user paths or unrelated logs;
- authentication-workaround binaries/components;
- third-party runtime binaries unless redistribution is explicitly permitted and the project has decided to carry them.

When sharing logs or screenshots, sanitize them first.

## Third-party projects

MCD2 Graphics depends on or interoperates with external projects including ReShade, RenoDX/UE Extended, Blueprint Loader, NeoRune and NVIDIA runtime/SDK components. OptiScaler work is currently exploratory.

When a defect is demonstrably in a third-party project, prefer a minimal upstream reproducer over a game-specific workaround. Keep game-specific integration changes in this repository.

## Pull requests

A useful pull request should include:

1. What changed.
2. Why the change belongs in this project.
3. Exact test environment.
4. What passed.
5. What remains untested or failed.
6. Any new dependency, redistribution or compatibility implication.

For risky renderer/lifecycle changes, include a native-fallback or rollback story.

## Hardware and platform qualification

Community validation is especially useful for:

- Windows;
- physical controllers;
- long sessions;
- RTX GPUs other than the maintainer-tested 4080 SUPER environment;
- additional resolutions/aspect ratios;
- future AMD/Intel provider paths if and when those are exposed.

Use the compatibility-report issue form for successful tests as well as failures. A successful report with complete environment details is useful evidence.
