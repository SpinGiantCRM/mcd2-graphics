"""Exercise the actual HDR-config function with synthetic addon/config state.

No game, GPU, vendor API, private memory or renderer INI is accessed.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile
import os
import re
import base64

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
#include "__ACTIVATION_HEADER__"
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
struct ActivationBackend {
 mcd2::hdr_activation::Key key;std::optional<mcd2::hdr_activation::Receipt> receipt;unsigned writes=0;
 bool read(mcd2::hdr_activation::Key&k){k=key;return true;}
 bool load(std::optional<mcd2::hdr_activation::Receipt>&r){r=receipt;return true;}
 bool save(std::optional<mcd2::hdr_activation::Receipt>r){receipt=r;return true;}
 bool write(const mcd2::hdr_activation::Key&expected,const mcd2::hdr_activation::Key&next){if(key!=expected)return false;key=next;++writes;return true;}
} activationBackend;
mcd2::hdr_activation::Activation activation;
auto update_hdr_addon_activation(bool hdr,bool resident){return activation.apply(hdr,resident,activationBackend);}
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
void apply_sdr_hdr_config(){__BODY__
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
 addon=true;apply_hdr_config();check(hdrRevision==0&&hdrRestart==1&&writes==0,"On token restoration requires restart even if addon is resident");
 activation={};config={{"ToneMapType",1},{"ToneMapPeakNits",410},{"ToneMapGameNits",203},{"ToneMapUINits",203}};
 apply_clean_hdr_config();check(hdrRevision==12&&hdrRestart==0&&writes==0&&wakes==3,"Fresh loaded calibration must acknowledge output");
 latestIntent.revision=13;latestIntent.peak=420;apply_clean_hdr_config();
 check(hdrRevision==13&&hdrRestart==1&&writes==1&&config.at("ToneMapPeakNits")==420,"Changed live calibration must require restart");
 apply_clean_hdr_config();check(writes==1&&wakes==4,"Repeated dirty revision must not write/wake again");
 latestIntent.revision=14;apply_clean_hdr_config();check(hdrRevision==14&&hdrRestart==1&&writes==1,"Dirty calibration must stay blocked for this process");
 // A separate function instance models a fresh process with SDR calibration.
 config={{"ToneMapType",0},{"ToneMapPeakNits",410},{"ToneMapGameNits",203},{"ToneMapUINits",203}};
 activation={};activationBackend={};latestIntent={20,0,410,203,203};auto before=writes;apply_sdr_hdr_config();
 check(hdrRevision==0&&hdrRestart==1,"Resident RenoDX must block HDR Off acknowledgement even with matching SDR tone mapper");
 check(activationBackend.receipt&&activationBackend.writes==1&&writes==before,"Off persists exact activation lease without changing calibration");
 apply_sdr_hdr_config();check(activationBackend.writes==1,"Repeated resident Off does not rewrite activation");
 latestIntent.revision=21;latestIntent.hdr=1;apply_sdr_hdr_config();
 check(hdrRevision==0&&hdrRestart==1&&writes==before&&!activationBackend.receipt,"Loaded SDR to HDR token restoration still needs restart");
 std::cout<<checks<<" actual-source HDR-config cases passed\n";
}
'''.replace('__BODY__', body).replace('__ACTIVATION_HEADER__', (args.source_root/'src/providers/hdr_addon_activation.hpp').as_posix())
    with tempfile.TemporaryDirectory(prefix='mcd2-hdr-gate-') as folder:
        folder = Path(folder)
        cpp, exe = folder / 'hdr_gate.cpp', folder / 'hdr_gate.exe'
        cpp.write_text(harness)
        subprocess.run([args.compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
        native=folder/'activation.exe'
        command=[args.compiler,'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',str(args.source_root/'tests/hdr_addon_activation_test.cpp'),'-o',str(native)]
        if os.name=='nt':command.insert(1,'-static')
        subprocess.run(command,check=True)
        receipt=folder/'native.bin';subprocess.run([str(native),'--receipt',str(receipt)],check=True)
        managed=(args.source_root/'tests/installer-core/Program.cs').read_text()
        golden=re.search(r'const string NativeHdrActivationReceipt="([^"]+)"',managed).group(1)
        assert receipt.read_bytes()==base64.b64decode(golden),'Native and managed activation receipt ABI changed'
        backend=text[text.index('struct HdrAddonBackend {'):text.index('\nmcd2::hdr_activation::Activation hdrAddonActivation;')]
        fixture=(args.source_root/'tests/hdr_addon_backend_fixture.cpp').read_text().replace('#include "../src/providers/hdr_addon_activation.hpp"','#include "'+(args.source_root/'src/providers/hdr_addon_activation.hpp').as_posix()+'"')
        cpp.write_text(fixture.replace('// ACTUAL_BACKEND',backend))
        subprocess.run([args.compiler,'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',str(cpp),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],cwd=folder,check=True)
        print('Actual backend: private-file WAL/cache recovery, empty key, external edits and redirection passed')


if __name__ == '__main__':
    main()
