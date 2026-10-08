"""Build an FG-Off startup trial, leaving the released SR/latency sources intact."""
from pathlib import Path
import argparse, hashlib, json, subprocess, sys

repo = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(repo))
from build_toolchain import native_frame_guard, normalize_windows_header

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--probe-output', required=True, type=Path)
p.add_argument('--reshade-headers', required=True, type=Path)
p.add_argument('--output', required=True, type=Path)
p.add_argument('--disable-factory-routing', action='store_true')
p.add_argument('--disable-sdk', action='store_true')
p.add_argument('--trace-nvapi', action='store_true')
p.add_argument('--mingw-cxx', default='x86_64-w64-mingw32-g++')
a = p.parse_args()
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=True)
receipt = json.loads((a.probe_output / 'build-receipt.json').read_text())
assert receipt['earlyBootstrap']
for name, expected in receipt['ownBinariesSHA256'].items():
    assert hashlib.sha256((a.probe_output / name).read_bytes()).hexdigest() == expected

original = (repo / 'src/latency/latency_addon.cpp').read_text()
source = original
changes = [
    ('  if(GetModuleHandleW(L"sl.interposer.dll"))return false;',
     '  if(!GetModuleHandleW(L"fg-sdk-bridge.dll"))return false;'),
    ('folder/L"mcd2-streamline-bridge.dll"', 'folder/L"fg-sdk-bridge.dll"'),
    ('  lastResult=init((folder/L"sl.interposer.dll").c_str(),folder.c_str(),logs.c_str());initialized=lastResult==0;if(!initialized)shutdown();return initialized;',
     '  auto ready=reinterpret_cast<int(*)()>(GetProcAddress(bridge,"mcd2_fg_initialized"));\n'
     '  lastResult=ready&&ready()?0:-100;initialized=lastResult==0;if(!initialized)shutdown();return initialized;'),
    ('std::filesystem::path(filename.get()).parent_path()/L"MCD2Graphics"/L"streamline"',
     'std::filesystem::path(filename.get()).parent_path()'),
    ('void shutdown(){std::lock_guard lock(configMutex);if(bridge&&stopSdk)stopSdk();',
     'std::atomic<int> shutdownResult{0};\n'
     ' void shutdown(bool stop=false){std::lock_guard lock(configMutex);if(stop&&bridge&&stopSdk){shutdownResult=stopSdk();if(shutdownResult)return;}'),
    ('provider.shutdown();reflexAvailable=0;',
     'provider.shutdown(boundNative!=0);\n'
     ' if(boundNative){log<<"{\\\"kind\\\":\\\"shared_sdk_shutdown\\\",\\\"SDKResult\\\":"<<provider.shutdownResult<<"}\\n";log.flush();\n'
     '  if(!provider.shutdownResult){auto bootstrap=GetModuleHandleW(L"dxgi.dll");auto detach=bootstrap?reinterpret_cast<void(*)()>(GetProcAddress(bootstrap,"mcd2_bootstrap_detach")):nullptr;if(detach)detach();}}\n'
     ' reflexAvailable=0;'),
    ('void init_swapchain(a::swapchain*swapchain,bool){',
     'void init(a::device*);\nvoid init_swapchain(a::swapchain*swapchain,bool){\n'
     ' auto ownerHwnd=static_cast<HWND>(swapchain->get_hwnd());DWORD ownerPid=0;wchar_t ownerClass[256]{};\n'
     ' GetWindowThreadProcessId(ownerHwnd,&ownerPid);GetClassNameW(ownerHwnd,ownerClass,256);\n'
     ' if(ownerPid!=GetCurrentProcessId()||wcscmp(ownerClass,L"UnrealWindow")!=0)return;\n'
     ' init(swapchain->get_device());'),
    ('reshade::register_event<reshade::addon_event::init_device>(init);',
     '/* SDK ownership is claimed only by the actual Unreal HWND swapchain. */'),
]
for old, new in changes:
    assert source.count(old) == 1, f'Unexpected source boundary: {old[:70]}'
    source = source.replace(old, new)
old_render = 'void render_start(void*,uint64_t id){record(2,id);forward(id,sr::Marker::RenderStart);}'
assert source.count(old_render) == 1
source = source.replace(old_render, '''std::atomic<uint64_t> fgLatestRenderIdentity{UINT64_MAX};
thread_local uint64_t fgThreadRenderIdentity=UINT64_MAX;
void render_start(void*,uint64_t id){fgThreadRenderIdentity=id;fgLatestRenderIdentity=id;record(2,id);forward(id,sr::Marker::RenderStart);}''')
source = source.replace('this bootstrap remains unqualified for FG until an earlier initialization path exists.',
                        'this experimental build adopts the earlier DXGI bootstrap; FG remains Off.')
source += '''
// Borrow the actual engine token under the existing bounded coordinator lock.
// The visitor must not wait for GPU work or re-enter the token coordinator.
extern "C" __declspec(dllexport) int mcd2_fg_visit_frame(uint64_t id,int(*visitor)(void*,unsigned,void*),void*context){
 auto c=controller();if(!installed||!c||!visitor)return -1;
 return c->with_token(id,[&](void*token,unsigned index){return visitor(token,index,context)==0;})?0:-2;
}
extern "C" __declspec(dllexport) uint64_t mcd2_fg_render_frame(){return installed&&base?*reinterpret_cast<uint64_t*>(base+engineLayout->renderCounter):UINT64_MAX;}
extern "C" __declspec(dllexport) uint64_t mcd2_fg_present_frame(){return installed&&presentActive?presentIdentity:UINT64_MAX;}
extern "C" __declspec(dllexport) uint64_t mcd2_fg_render_marker(){return installed?fgLatestRenderIdentity.load():UINT64_MAX;}
extern "C" __declspec(dllexport) uint64_t mcd2_fg_thread_render_marker(){return installed?fgThreadRenderIdentity:UINT64_MAX;}
// SR's successful feature retirement notifies this module outside its mutex.
// Drain engine tokens before stopping the shared SDK while the device is valid.
extern "C" __declspec(dllexport) int mcd2_fg_quiesce(){cleanup();return provider.shutdownResult;}
'''
(out/'latency_candidate.cpp').write_text(source)
(out/'FGBootstrap.ini').write_text('[Experiment]\nEnableSDK='+str(int(not a.disable_sdk))+'\nFactoryRouting='+str(int(not a.disable_factory_routing))+'\nTraceNvapi='+str(int(a.trace_nvapi))+'\n')
inc = out/'include'
inc.mkdir(exist_ok=True)
for file in a.reshade_headers.glob('reshade*.hpp'):
    (inc/file.name).write_bytes(normalize_windows_header(file.read_bytes()))
command = [a.mingw_cxx, '-std=c++20', '-O2',
           *native_frame_guard(a.mingw_cxx), '-shared', '-static', '-s',
           '-I'+str(inc), '-I'+str(repo/'src/latency'), str(out/'latency_candidate.cpp'),
           '-o', str(out/'mcd2-display-latency.addon64'),
           '-ld3d12', '-ldxgi', '-ld3dcompiler', '-lole32', '-luuid']
with (out/'build-private.log').open('w') as log:
    subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
(out/'build-receipt.json').write_text(json.dumps({
    'purpose':'FG-Off game bootstrap adoption only', 'FGEnabled':False,
    'SDKEnabled':not a.disable_sdk, 'FactoryRouting':not a.disable_factory_routing,
    'baseSourceSHA256':hashlib.sha256(original.encode()).hexdigest(),
    'generatedSourceSHA256':hashlib.sha256(source.encode()).hexdigest(),
    'engineLayoutSHA256':hashlib.sha256((repo/'src/latency/engine_layout.hpp').read_bytes()).hexdigest(),
    'amdLatencySHA256':hashlib.sha256((repo/'src/latency/amd_latency.hpp').read_bytes()).hexdigest(),
    'providerHeadersSHA256':{path.name:hashlib.sha256(path.read_bytes()).hexdigest()
                             for path in sorted((repo/'src/providers').glob('*.hpp'))},
    'addonSHA256':hashlib.sha256((out/'mcd2-display-latency.addon64').read_bytes()).hexdigest(),
    'probeBuild':receipt, 'WindowsQualified':False,
}, indent=2)+'\n')
print('Experimental latency adapter built; FG remains disabled. Released sources unchanged.')
