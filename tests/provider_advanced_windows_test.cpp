#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../src/latency/advanced_settings_transport.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
int main(){
    const auto folder=std::filesystem::temp_directory_path()/(L"mcd2-advanced-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
    assert(!std::filesystem::exists(folder));std::filesystem::create_directory(folder);
    const auto path=folder/L"DependencyOverrides.ini";
    {std::ofstream f(path);f<<"[Overrides]\nAllowUntestedReShade=1\n[DLSS]\nModelPreset=12\nKeepMe=unchanged\n";}
    mcd2::advanced::IniHint ini{path};unsigned hint=0;assert(ini.read(hint)&&hint==12);
    assert(ini.write(19)&&ini.read(hint)&&hint==19);assert(!ini.write(256));
    wchar_t text[64]{};GetPrivateProfileStringW(L"Overrides",L"AllowUntestedReShade",L"",text,64,path.c_str());assert(std::wstring(text)==L"1");
    GetPrivateProfileStringW(L"DLSS",L"KeepMe",L"",text,64,path.c_str());assert(std::wstring(text)==L"unchanged");
    assert(WritePrivateProfileStringW(L"DLSS",L"ModelPreset",L"invalid",path.c_str()));assert(!ini.read(hint));
    assert(WritePrivateProfileStringW(L"DLSS",L"ModelPreset",L"256",path.c_str()));assert(!ini.read(hint));
    assert(WritePrivateProfileStringW(L"DLSS",L"ModelPreset",L"0",path.c_str()));assert(ini.read(hint)&&hint==0);
    mcd2::advanced::IniHint fg{ini.path,L"FrameGenerationPreset",2};
    assert(fg.write(2));unsigned fgValue=0;assert(fg.read(fgValue)&&fgValue==2);
    assert(!fg.write(3));mcd2::advanced::IniHint fg3{ini.path,L"FrameGenerationPreset",3};assert(fg3.write(3)&&fg3.read(fgValue)&&fgValue==3);
    mcd2::advanced::IniHint fsr{ini.path,L"UpscalingVersion",2,L"FSR"};assert(fsr.write(2));unsigned fsrValue=0;assert(fsr.read(fsrValue)&&fsrValue==2);assert(!fsr.write(3));assert(fg3.read(fgValue)&&fgValue==3);unsigned modelAfter=0;assert(ini.read(modelAfter)&&modelAfter==0);
    mcd2::providers::AdvancedState state;
    const auto missing=folder/L"missing-provider.dll";assert(!GetModuleHandleW(missing.c_str()));
    mcd2::advanced::moduleVersion(missing,0,state);assert(state.modules==0&&!GetModuleHandleW(missing.c_str()));
    auto kernel=GetModuleHandleW(L"kernel32.dll");wchar_t module[32768]{};assert(GetModuleFileNameW(kernel,module,32768));
    mcd2::advanced::moduleVersion(module,1,state);assert(state.modules&2);
    assert(GetModuleHandleW(L"kernel32.dll")==kernel);
    mcd2::advanced::moduleVersion(module,4,state);assert(state.modules&256);
    assert(GetModuleHandleW(L"kernel32.dll")==kernel);
    auto before=state.modules;mcd2::advanced::moduleVersion(module,5,state);assert(state.modules==before);
    std::filesystem::remove_all(folder);
    std::cout<<"Advanced Windows INI: single-key writes, override retention, invalid values, exact loaded-module metadata and missing-module no-load checks pass\n";
}
