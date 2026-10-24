#pragma once
#include <filesystem>
#include <cstdint>
#include <optional>
#include <string>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <vector>
#endif
namespace mcd2::providers {
enum class SettingsEdition { Unknown,Steam,WinGDK };
struct SettingsPaths {std::filesystem::path saves,authority;};
inline std::optional<SettingsPaths> settingsPaths(const std::filesystem::path& local,SettingsEdition edition,const std::filesystem::path& boundCache){
    const auto saved=local/"Dungeons2"/"Saved";
    if(edition==SettingsEdition::Steam)return SettingsPaths{saved/"SaveGames",saved/"MCD2Graphics"};
    if(edition!=SettingsEdition::WinGDK||boundCache.empty()||boundCache!=boundCache.lexically_normal())return {};
    const auto binding=boundCache.parent_path().filename().string();
    if(binding.size()!=64||!std::all_of(binding.begin(),binding.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');})||
       boundCache.parent_path().parent_path()!=saved/"MCD2Graphics"/"WinGDK"||boundCache.filename()!="SaveGames")return {};
    return SettingsPaths{boundCache,boundCache.parent_path()/"MCD2Graphics"};
}
#ifdef _WIN32
inline std::optional<SettingsPaths> currentSettingsPaths(const std::filesystem::path& local){
    std::vector<wchar_t> exe(32768);auto n=GetModuleFileNameW(nullptr,exe.data(),DWORD(exe.size()));if(!n||n>=exe.size())return {};
    const auto name=std::filesystem::path(exe.data()).filename().wstring();
    if(_wcsicmp(name.c_str(),L"Dungeons-Win64-Shipping.exe")==0)return settingsPaths(local,SettingsEdition::Steam,{});
    if(_wcsicmp(name.c_str(),L"Dungeons-WinGDK-Shipping.exe")!=0)return {};
    auto bridge=GetModuleHandleW(L"mcd2-gdk-save-bridge.addon64");
    using Root=int(*)(wchar_t*,std::uint32_t);
    auto get=bridge?reinterpret_cast<Root>(GetProcAddress(bridge,"mcd2_gdk_save_root_v1")):nullptr;
    std::vector<wchar_t> cache(32768);
    if(!get||get(cache.data(),std::uint32_t(cache.size()))!=0)return {};
    return settingsPaths(local,SettingsEdition::WinGDK,std::filesystem::path(cache.data()));
}
#endif
}
