from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ProcessExitTests(unittest.TestCase):
    def test_ngx_unshared_shutdown_uses_the_initialization_device(self):
        source = (ROOT / 'src/native/ngx_live_fixture.hpp').read_text()
        cleanup = source.split('static bool cleanup_detached_live(', 1)[1].split('static void cleanup_live_saved(', 1)[0]
        self.assertIn('cache_path.c_str(),c.proxy_device,NVSDK_NGX_Version_API', source)
        self.assertIn('!c.proxy_device||!result("shutdown_device",fn(c.proxy_device))', cleanup)
        self.assertNotIn('fn(c.resource_device)', cleanup)
        self.assertNotIn('fn(nullptr)', cleanup)

    def test_ngx_shutdown_preserves_shared_owner_and_resource_retirement(self):
        source = (ROOT / 'src/native/ngx_live_fixture.hpp').read_text()
        cleanup = source.split('static bool cleanup_detached_live(', 1)[1].split('static void cleanup_live_saved(', 1)[0]
        self.assertIn('owner(c.proxy_device)', cleanup)
        self.assertIn('if(ownership<0)', cleanup)
        self.assertIn('if(ownership==1)', cleanup)
        self.assertIn('shutdown_deferred_to_streamline', cleanup)
        self.assertLess(cleanup.index('release_feature'), cleanup.index('shutdown_device'))
        self.assertLess(cleanup.index('destroy_parameters'), cleanup.index('shutdown_device'))
        self.assertLess(cleanup.index('shutdown_device'), cleanup.index('c.owned.resources.rbegin()'))
        self.assertLess(cleanup.index('c.owned.resources.clear()'), cleanup.index('FreeLibrary(c.module)'))
        lean = (ROOT / 'src/native/ngx_lean.hpp').read_text()
        self.assertLess(lean.index('success=live_wait_generation(q,*context)'),
                        lean.index('success=cleanup_detached_live(*context)'))

    def test_sr_terminal_guard_preserves_ordinary_cleanup(self):
        source = (ROOT / 'src/native/observer.cpp').read_text()
        self.assertIn('mcd2::process_exit::initialize();', source.split('static void init_device(', 1)[1])
        queue = source.split('static void destroy_queue(', 1)[1].split('static void destroy_device(', 1)[0]
        self.assertLess(queue.index('if(mcd2::process_exit::terminating())return;'), queue.index('std::unique_lock guard(lock)'))
        self.assertIn('lean_cleanup(q,guard,true)', queue)
        detach = source.split('else if(reason==DLL_PROCESS_DETACH)', 1)[1]
        self.assertLess(detach.index('if(reserved){mcd2::process_exit::mark_terminating();return TRUE;}'), detach.index('REMOVE(init_device'))
        self.assertIn('reshade::unregister_addon(module)', detach)
        for filename, typename, name in (('native_fg_guides.hpp', 'Generation', 'generation'),
                                         ('fsr_runtime.hpp', 'Generation', 'generation'),
                                         ('ngx_live_fixture.hpp', 'LiveFixture', 'live_fixture')):
            header = (ROOT / 'src/native' / filename).read_text()
            self.assertIn(f'Lifetime<std::unique_ptr<{typename}>,retain_sr_generation> {name}Lifetime', header)
            self.assertIn(f'{name}Lifetime.get()', header)

    def test_retained_generation_unload_pins_both_modules_outside_dllmain(self):
        source = (ROOT / 'src/native/observer.cpp').read_text()
        uninit = source.split('void AddonUninit(', 1)[1].split('BOOL WINAPI DllMain(', 1)[0]
        self.assertIn('if(mcd2::process_exit::terminating())return;', uninit)
        for owner in ('live_fixture', 'fsr_runtime::generation', 'native_fg::generation'):
            self.assertIn(f'bool({owner})', uninit)
        self.assertIn('if(!retained)return;', uninit)
        self.assertIn('GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN', uninit)
        self.assertIn('reinterpret_cast<LPCWSTR>(module)', uninit)
        self.assertIn('reinterpret_cast<LPCWSTR>(framework)', uninit)
        self.assertIn('reshade::unregister_addon(module)', uninit)
        self.assertIn('return !unload_generation_retained;', source)
        self.assertIn('return unload_generation_retained||mcd2::process_exit::terminating();', source)
        self.assertIn('heldAddon==module&&heldFramework==framework', uninit)
        self.assertNotIn('GetModuleHandleExW', source.split('BOOL WINAPI DllMain(', 1)[1])
        for forbidden in ('Release()', 'wait_idle()', 'ExitProcess(', 'TerminateProcess(', 'Shutdown1(', 'Reset('):
            self.assertNotIn(forbidden, uninit)

    def test_native_guide_manual_release_clears_destructor_ownership(self):
        source = (ROOT / 'src/native/native_fg_guides.hpp').read_text()
        release = source.split('auto retired=std::move(generation)', 1)[1].split('++retirements', 1)[0]
        self.assertLess(release.index('(*it)->Release()'), release.index('owned.resources.clear()'))
        self.assertLess(release.index('owned.resources.clear()'), release.index('retired.reset()'))
        recipe = (ROOT / 'experiments/fg-streamline/build_sr_candidate.py').read_text()
        self.assertIn('terminalGuardSourceSHA256', recipe)

    def test_terminal_guards_precede_locks_and_sdk_cleanup(self):
        source = (ROOT / 'src/latency/latency_addon.cpp').read_text()
        self.assertIn('void cleanup(){\n if(mcd2::process_exit::terminating())return;', source)
        self.assertIn('void AddonUninit(HMODULE,HMODULE){\n if(mcd2::process_exit::terminating())return;', source)
        self.assertIn('if(reserved)mcd2::process_exit::mark_terminating();else reshade::unregister_addon(h)', source)
        # Ordinary unload keeps its existing worker join, SDK drains and unload.
        self.assertIn('if(settingsWorker.joinable())settingsWorker.join();cleanup();amdProvider.unload();', source)
        self.assertIn('if(c){c->shutdown();c->drain();}provider.shutdown();', source)
        for name in ('coordinatorLifetime', 'amdCoordinatorLifetime', 'settingsWorkerLifetime'):
            self.assertIn(name, source)

    def test_query_is_resolved_before_callbacks_not_in_dllmain(self):
        source = (ROOT / 'src/latency/latency_addon.cpp').read_text()
        self.assertIn('bool AddonInit(HMODULE module,HMODULE){\n mcd2::process_exit::initialize();', source)
        header = (ROOT / 'src/latency/process_exit.hpp').read_text()
        self.assertIn('RtlDllShutdownInProgress', header)
        self.assertNotIn('LoadLibrary', header)
        self.assertNotIn('WaitFor', header)
        self.assertNotIn('Sleep(', header)
        self.assertNotIn('ExitProcess(', header)

    def test_generated_candidate_retains_guards_and_header_provenance(self):
        recipe = (ROOT / 'experiments/fg-streamline/build_game_candidate.py').read_text()
        self.assertIn("('process_exit.hpp','process_lifetime.hpp')", recipe)
        self.assertNotIn("source.replace(' if(mcd2::process_exit::terminating())return;'", recipe)


if __name__ == '__main__':
    unittest.main()
