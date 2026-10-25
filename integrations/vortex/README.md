# MCD2 Graphics Vortex companion

Development integration for Vortex 2.9.0+, alongside the existing Minecraft Dungeons II game extension. See [installation and manager transitions](../../docs/INSTALLER_MANAGERS.md). Nothing here qualifies a game runtime or provides benchmark results.

The package installer recognises `mcd2-install.json` and complete hash-pinned requirement releases, emits `mcd2-graphics-root` managed copy instructions, and leaves deployment to Vortex. The stock `hardlink_activator` keeps its identity and deployment manifests. Through the public `util.getActivator` API, the companion decorates its directly awaited transaction methods to run preflight and configuration restoration. Other game roots delegate without MCD2 work.

This deliberately avoids async `will-deploy`/`will-purge` listeners: Vortex 2.9.0's `emitAndAwait` catches their errors and continues. Do not move blocking checks into those listeners. The boundary fixtures reject work before underlying hardlink operations, and use real temporary files and links for successful deploy/disable/re-enable/purge and configuration recovery. Live Vortex qualification is still required.

```sh
npm --prefix integrations/vortex test
python tools/build_vortex_companion.py --output dist/MCD2-Graphics-Companion.zip
```

The companion ZIP includes only our JavaScript and metadata. It does not include the community extension, vendor runtimes, Node or .NET. Install this as an extension; install the separate Vortex payload ZIP as a game mod. Keep the companion enabled until its game packages have been purged.

Upstream API evidence: [public util exports](https://github.com/Nexus-Mods/Vortex/blob/v2.9.0/src/renderer/src/util/api.ts), [deployment-method interface](https://github.com/Nexus-Mods/Vortex/blob/v2.9.0/src/renderer/src/extensions/mod_management/types/IDeploymentMethod.ts), [purge calls](https://github.com/Nexus-Mods/Vortex/blob/v2.9.0/src/renderer/src/extensions/mod_management/util/deploy.ts), [async event handling](https://github.com/Nexus-Mods/Vortex/blob/v2.9.0/src/renderer/src/ExtensionManager.ts).
