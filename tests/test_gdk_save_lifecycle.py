from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class GdkSaveLifecycleTests(unittest.TestCase):
    def test_only_the_actual_title_window_claims_shutdown(self):
        source = (ROOT / 'src/gdk/save_bridge_addon.cpp').read_text(encoding='utf-8')
        init = source.split('void swap_init(', 1)[1].split('void swap_destroy(', 1)[0]
        self.assertIn('pid!=GetCurrentProcessId()', init)
        self.assertIn('std::wcscmp(name,L"UnrealWindow")', init)
        self.assertIn('if(worker&&!saveOwner)', init)
        destroy = source.split('void swap_destroy(', 1)[1].split('std::atomic<uint32_t>', 1)[0]
        self.assertLess(destroy.index('terminating()'), destroy.index('lifecycleMutex'))
        self.assertIn('closesSaveOwner(false,resize,saveOwner', destroy)
        self.assertIn('if(close)stopWorker();', destroy)

    def test_stop_wakes_and_joins_before_deleting_worker(self):
        source = (ROOT / 'src/gdk/save_bridge_addon.cpp').read_text(encoding='utf-8')
        stop = source.split('void stopWorker()', 1)[1].split('void swap_init(', 1)[0]
        self.assertLess(stop.index('worker->stop=true'), stop.index('wake.notify_all()'))
        self.assertLess(stop.index('wake.notify_all()'), stop.index('thread.join()'))
        self.assertLess(stop.index('thread.join()'), stop.index('delete worker'))
        uninit = source.split('void AddonUninit(', 1)[1]
        self.assertLess(uninit.index('terminating()'), uninit.index('stopWorker()'))

    def test_save_handles_close_before_own_runtime_reference(self):
        source = (ROOT / 'src/gdk/game_save_api.hpp').read_text(encoding='utf-8')
        self.assertIn('if(provider)api.closeProvider(provider);if(user)api.closeUser(user);', source)
        self.assertIn('if(initialized)stop();if(module)FreeLibrary(module);', source)
        self.assertIn('Api api;XUserHandle user', source)


if __name__ == '__main__':
    unittest.main()
