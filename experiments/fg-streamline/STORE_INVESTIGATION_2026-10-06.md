# Xbox app / Microsoft Store PC investigation

## Current evidence

[Issue #8](https://github.com/SpinGiantCRM/mcd2-graphics/issues/8) reports a
`Content/Dungeons/Binaries/WinGDK` installation. As of 6 October, no shipping
executable filename/hash/build or qualified runtime evidence has been supplied.
The installer supports the fingerprinted Steam Win64 build. No Store support
has been enabled, and no game executable was obtained or uploaded for this work.

Microsoft's [Flat File Install documentation](https://learn.microsoft.com/en-us/gaming/gdk/docs/features/common/packaging/packaging-flatfileinstall)
says PC GDK game resources are modifiable, while designated executables can
remain encrypted. Its [mod support documentation](https://learn.microsoft.com/en-us/gaming/gdk/docs/features/common/packaging/packaging-mods)
describes resource modification by default. Neither establishes this mod's
compatibility with a particular game's protected executable or loading path.

Microsoft also [documents](https://developer.microsoft.com/en-us/games/articles/2026/06/building-xbox-games-with-unreal-engines-new-gdk-plug-ins/)
the newer Unreal 5.8 Win64 GDK path and the older separate WinGDK platform.
That explains why storefront naming alone is not a reliable binary/ABI test;
it does not establish which implementation MCD2's installed Store build uses.

## Findings integrated

`store_probe.py` is a read-only Windows/Linux comparison tool. It records only
the filename, optional version, full-file hash, PE architecture/image size,
code-section hash and Boolean matches for the five existing Steam Reflex
boundaries. It does not emit code, absolute paths, account data or saves.

```text
python experiments/fg-streamline/store_probe.py --exe SHIPPING_EXE --output new-store-comparison.json
```

Run on a legitimately installed PC build. If the OS denies reading or a readable
PE is unavailable, retain that result; do not decrypt, change permissions, rename
the binary or disable game licensing. The output cannot enable installation.
Even five matching boundary checks do not validate the modular registry ABI,
render resources, shader signatures, generated UI classes or save locations.

The tool is qualified against the readable Steam executable and synthetic
malformed/valid PE fixtures. The exact verified Steam file does **not** match
the five on-disk byte checks, despite those runtime checks passing in the FG
game trial. Therefore file-byte comparison is not a substitute for the loaded
engine checks. The cause of that disk/runtime difference is not established;
no protection/decryption assumption or workaround is made. Real WinGDK execution remains untested.
Installer/runtime allowlists and all released binaries are unchanged.

## Windows follow-up

1. Record store, game build and the sanitized comparison from an installed PC
   build; keep the executable and raw logs local.
2. Check Blueprint Loader/NeoRune and ReShade loading independently of this mod.
3. Determine the actual mod save/config locations without reading player saves.
4. If accessible and supported, verify the Store modular-feature registry,
   View/camera layout, SR/HDR compositor signatures and resource lifetimes.
5. Only after those gates, implement a separately fingerprinted Store profile
   and a reversible installer mapping. Qualify AMD fallback first, then NVIDIA
   SR/Reflex/FG/HDR and normal teardown.

**Disposition:** ready for Windows metadata collection; Store runtime support
remains blocked by the absence of a verified installed-build comparison.
The FG candidate is a bounded Steam/NVIDIA experiment. No Store claim is made.
