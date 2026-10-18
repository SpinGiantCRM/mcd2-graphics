"""Exercise the actual bootstrap trace writer against concurrent route teardown.

Synthetic file and route objects only; no game, GPU, SDK or player files.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--source-root', type=Path, default=ROOT)
    args = parser.parse_args()
    source = (args.source_root / 'experiments/fg-streamline/bootstrap_dxgi.cpp').read_text()
    writer = function(source, 'void event(const char* stage,long result){')
    detach = function(source, 'extern "C" __declspec(dllexport) void mcd2_bootstrap_detach(){')
    harness = r'''
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>
using namespace std::chrono_literals;
struct TraceFile {bool open=true;unsigned writes=0,closes=0,violations=0;};
TraceFile file;
TraceFile* receipt=&file;
std::mutex routesMutex,stateMutex;
std::condition_variable changed;
bool writerPaused=false,releaseWriter=false,detached=false,closeFinished=false;
unsigned traceLockAttempts=0;
struct TraceMutex {
 std::mutex value;
 void lock(){
  {std::lock_guard lock(stateMutex);++traceLockAttempts;changed.notify_all();}
  value.lock();
 }
 void unlock(){value.unlock();}
} receiptMutex;
struct Route {void detach(){std::lock_guard lock(stateMutex);detached=true;changed.notify_all();}};
std::array<Route,1> routes;
int trace_fprintf(TraceFile* stream,const char*,const char* stage,long){
 std::unique_lock lock(stateMutex);
 if(!std::strcmp(stage,"paused-writer")){
  writerPaused=true;changed.notify_all();changed.wait(lock,[]{return releaseWriter;});
 }
 if(!stream->open)++stream->violations;
 ++stream->writes;return 1;
}
int trace_fflush(TraceFile* stream){
 // fflush(nullptr) legally flushes all streams; a non-null closed handle is invalid.
 std::lock_guard lock(stateMutex);if(stream&&!stream->open)++file.violations;return 0;
}
int trace_fclose(TraceFile* stream){
 std::lock_guard lock(stateMutex);if(!stream->open)++stream->violations;
 stream->open=false;++stream->closes;return 0;
}
#define fprintf trace_fprintf
#define fflush trace_fflush
#define fclose trace_fclose
#ifndef _WIN32
#define __declspec(value)
#endif
__WRITER__
__DETACH__
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(2);}}
int main(){
 std::thread writer([]{event("paused-writer",0);});
 {
  std::unique_lock lock(stateMutex);
  check(changed.wait_for(lock,5s,[]{return writerPaused;}),"Writer did not reach the controlled boundary");
 }
 std::thread closer([]{mcd2_bootstrap_detach();std::lock_guard lock(stateMutex);closeFinished=true;changed.notify_all();});
 {
  std::unique_lock lock(stateMutex);
  check(changed.wait_for(lock,5s,[]{return detached;}),"Teardown did not reach route retirement");
  // Old teardown closes under the paused write; fixed teardown reaches its
  // second trace-lock attempt and blocks. Observe either boundary explicitly,
  // without using a scheduling delay to decide whether the race occurred.
  check(changed.wait_for(lock,5s,[]{return closeFinished||traceLockAttempts>=2;}),"Closer did not reach the controlled boundary");
  releaseWriter=true;changed.notify_all();
 }
 writer.join();closer.join();
 check(file.violations==0,"Bootstrap teardown closed a trace still used by another caller");
 check(file.closes==1&&receipt==nullptr,"Trace ownership must clear after one close");
 check(file.writes==2,"Both admitted events must complete before close");
 event("late-writer",0);mcd2_bootstrap_detach();
 check(file.writes==2&&file.closes==1,"Late events/repeated teardown must not reuse a closed trace");
 std::cout<<"Actual bootstrap trace: concurrent write/close and repeated teardown passed\n";
}
'''.replace('__WRITER__', writer).replace('__DETACH__', detach)
    with tempfile.TemporaryDirectory(prefix='mcd2-bootstrap-trace-') as directory:
        folder = Path(directory)
        cpp, exe = folder / 'trace.cpp', folder / 'trace.exe'
        cpp.write_text(harness)
        subprocess.run([args.compiler, '-std=c++20', '-pthread', '-Wall', '-Wextra',
                        '-Werror', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True, timeout=15)


if __name__ == '__main__':
    main()
