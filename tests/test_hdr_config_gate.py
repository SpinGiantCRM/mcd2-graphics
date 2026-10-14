"""Exercise the actual HDR-config function with synthetic addon/config state.

No game, GPU, vendor API, private memory or renderer INI is accessed.
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
    text = (args.source_root / 'src/latency/latency_addon.cpp').read_text()
    body = text.split('void apply_hdr_config(){', 1)[1].split('\nvoid destroy_swapchain(', 1)[0]
    assert body.rstrip().endswith('}')
    harness = r'''
#include <array>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <iostream>
#include <cstdlib>
namespace mcd2::display {
struct Intent {unsigned revision=0,hdr=0,peak=410,paper=203,ui=203;};
}
namespace {
std::mutex intentMutex;
mcd2::display::Intent latestIntent;
unsigned hdrRevision=0,hdrRestart=0,wakes=0,writes=0,checks=0;
bool addon=false;
std::ostringstream log;
struct Wake {void notify_all(){++wakes;}} workerWake;
void* GetModuleHandleW(const wchar_t*){return addon?reinterpret_cast<void*>(1):nullptr;}
std::map<std::string,float> config;
namespace reshade {
void get_config_value(void*,const char*,const char* key,unsigned& value){
 if(std::string(key)=="SelectedProfile")value=1;
}
void get_config_value(void*,const char*,const char* key,float& value){
 if(config.count(key))value=config.at(key);
}
void set_config_value(void*,const char*,const char* key,unsigned value){
 config[key]=float(value);++writes;
}
}
void apply_hdr_config(){__BODY__
void apply_clean_hdr_config(){__BODY__
void check(bool value,const char* message){
 ++checks;if(!value){std::cerr<<message<<"\n";std::exit(2);}
}
}
int main(){
 apply_hdr_config();check(hdrRevision==0&&hdrRestart==0&&wakes==0,"No valid intent must leave output untouched");
 latestIntent.revision=10;apply_hdr_config();
 check(hdrRevision==10&&hdrRestart==0,"HDR Off without RenoDX must acknowledge native output change");
 check(wakes==1&&writes==0,"Missing addon Off must not write calibration");
 apply_hdr_config();check(wakes==1&&writes==0,"Repeated Off revision must not wake/write every Present");
 latestIntent.revision=11;apply_hdr_config();check(hdrRevision==11&&hdrRestart==0&&wakes==2,"New Off revision must be acknowledged");
 latestIntent.revision=12;latestIntent.hdr=1;apply_hdr_config();
 check(hdrRevision==0&&hdrRestart==1&&wakes==2&&writes==0,"Missing addon HDR On must remain blocked");
 apply_hdr_config();check(hdrRevision==0&&hdrRestart==1&&wakes==2,"Repeated missing-addon On must not wake the worker");
 addon=true;config={{"ToneMapType",1},{"ToneMapPeakNits",410},{"ToneMapGameNits",203},{"ToneMapUINits",203}};
 apply_hdr_config();check(hdrRevision==12&&hdrRestart==0&&writes==0&&wakes==3,"Matching loaded calibration must acknowledge output");
 latestIntent.revision=13;latestIntent.peak=420;apply_hdr_config();
 check(hdrRevision==13&&hdrRestart==1&&writes==1&&config.at("ToneMapPeakNits")==420,"Changed live calibration must require restart");
 apply_hdr_config();check(writes==1&&wakes==4,"Repeated dirty revision must not write/wake again");
 latestIntent.revision=14;apply_hdr_config();check(hdrRevision==14&&hdrRestart==1&&writes==1,"Dirty calibration must stay blocked for this process");
 // A separate function instance models a fresh process with SDR calibration.
 config={{"ToneMapType",0},{"ToneMapPeakNits",410},{"ToneMapGameNits",203},{"ToneMapUINits",203}};
 latestIntent={20,0,410,203,203};auto before=writes;apply_clean_hdr_config();
 check(hdrRevision==20&&hdrRestart==0&&writes==before,"Matching loaded SDR calibration must preserve old admission");
 latestIntent.revision=21;latestIntent.hdr=1;apply_clean_hdr_config();
 check(hdrRevision==21&&hdrRestart==1&&writes==before+1,"Loaded SDR to HDR still needs restart");
 std::cout<<checks<<" actual-source HDR-config cases passed\n";
}
'''.replace('__BODY__', body)
    with tempfile.TemporaryDirectory(prefix='mcd2-hdr-gate-') as folder:
        folder = Path(folder)
        cpp, exe = folder / 'hdr_gate.cpp', folder / 'hdr_gate.exe'
        cpp.write_text(harness)
        subprocess.run([args.compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
