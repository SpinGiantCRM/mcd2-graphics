"""Execute the real owned-resource destructor and FSR retirement release block.

Synthetic COM counters only: no game, SDK, GPU or process inspection.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--source-root', type=Path, default=ROOT)
    args = parser.parse_args()
    owner = (args.source_root / 'src/native/ngx_saved_eval.hpp').read_text()
    owner = 'struct EvalOwned {' + owner.split('struct EvalOwned {', 1)[1].split('\n};', 1)[0] + '\n};'
    source = (args.source_root / 'src/native/fsr_runtime.hpp').read_text()
    release = source.split('auto retired=std::move(generation);', 1)[1].split('guard.lock();', 1)[0]
    harness = r'''
#include <array>
#include <memory>
#include <vector>
#include <iostream>
static std::vector<int> events;
struct IUnknown {
 int id, refs=2, releases=0;
 void Release(){--refs;++releases;events.push_back(id);}
};
__OWNER__
struct Borrows {void clear_after_idle(){events.push_back(100);}};
struct Generation {EvalOwned owned;Borrows borrows;void* bridge=nullptr;};
struct Guard {void unlock(){};} guard;
static void FreeLibrary(void*){events.push_back(200);}
static bool run(unsigned count,bool duplicate,bool bridge){
 events.clear();std::array<IUnknown,3> resources{{{1},{2},{3}}};
 auto generation=std::make_unique<Generation>();
 for(unsigned i=0;i<count;++i)generation->owned.keep(&resources[i]);
 if(duplicate){++resources[0].refs;generation->owned.keep(&resources[0]);}
 generation->owned.resources.push_back(nullptr);
 generation->bridge=bridge?reinterpret_cast<void*>(1):nullptr;
 auto retired=std::move(generation);
 __RELEASE__
 if(generation||retired)return false;
 for(unsigned i=0;i<resources.size();++i){
  int expected=int(i<count)+(duplicate&&i==0);
  if(resources[i].releases!=expected||resources[i].refs!=(i<count?1:2))return false;
 }
 std::vector<int> expected{100};if(duplicate)expected.push_back(1);
 for(unsigned i=count;i>0;--i)expected.push_back(int(i));
 if(bridge)expected.push_back(200);
 return events==expected;
}
int main(){
 for(unsigned count=0;count<=3;++count)for(bool bridge:{false,true}){
  if(!run(count,false,bridge)){std::cerr<<"owned reference released twice or wrong retirement order\n";return 1;}
  if(count&&!run(count,true,bridge)){std::cerr<<"duplicate reference ownership failed\n";return 2;}
 }
 std::cout<<"14 FSR ownership/destructor cases passed\n";
}
'''.replace('__OWNER__', owner).replace('__RELEASE__', release)
    with tempfile.TemporaryDirectory(prefix='mcd2-owned-retirement-') as folder:
        folder = Path(folder)
        cpp, exe = folder / 'retirement.cpp', folder / 'retirement.exe'
        cpp.write_text(harness)
        subprocess.run([args.compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
