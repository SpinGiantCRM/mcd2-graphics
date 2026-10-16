from pathlib import Path
import unittest
import shutil
import subprocess
import tempfile
import sys

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

    def test_actual_worker_stop_wakes_joins_and_is_idempotent(self):
        source = (ROOT / 'src/gdk/save_bridge_addon.cpp').read_text(encoding='utf-8')
        worker = 'struct Worker {' + source.split('struct Worker {', 1)[1].split('void swap_init(', 1)[0]
        harness = """
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
""" + worker + """
int main(){
    std::atomic<bool> waiting=false,finished=false;
    worker=new Worker;auto* state=worker;
    state->thread=std::thread([&]{
        std::unique_lock lock(state->mutex);waiting=true;
        state->wake.wait_for(lock,std::chrono::seconds(5),[&]{return state->stop.load();});
        assert(state->stop);finished=true;
    });
    while(!waiting)std::this_thread::yield();
    stopWorker();assert(!worker&&finished);stopWorker();
}
"""
        compiler = shutil.which('g++') or shutil.which('clang++')
        self.assertIsNotNone(compiler, 'C++ compiler required by both CI platforms')
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);cpp = root / 'worker.cpp';exe = root / 'worker.exe'
            cpp.write_text(harness, encoding='utf-8')
            subprocess.run([compiler, '-std=c++20', *(['-static'] if sys.platform == 'win32' else []), '-pthread', str(cpp), '-o', str(exe)], check=True, capture_output=True)
            subprocess.run([str(exe)], check=True, timeout=3, capture_output=True)

    def test_save_handles_close_before_own_runtime_reference(self):
        source = (ROOT / 'src/gdk/game_save_api.hpp').read_text(encoding='utf-8')
        self.assertIn('if(provider)api.closeProvider(provider);if(user)api.closeUser(user);', source)
        self.assertIn('if(initialized)stop();if(module)FreeLibrary(module);', source)
        self.assertIn('Api api;XUserHandle user', source)


if __name__ == '__main__':
    unittest.main()
