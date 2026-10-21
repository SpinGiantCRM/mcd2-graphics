#pragma once
#include <windows.h>
#include <filesystem>
#include <vector>

namespace mcd2::ngx {
inline HMODULE load_driver(){
 if(auto module=LoadLibraryW(L"nvngx.dll"))return module;
 // Windows installs NGX in the registered driver directory, outside DLL search paths.
 std::vector<wchar_t> directory(32768);DWORD bytes=DWORD(directory.size()*sizeof(wchar_t));
 if(RegGetValueW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\NVIDIA Corporation\\Global\\NGXCore",L"FullPath",
                 RRF_RT_REG_SZ,nullptr,directory.data(),&bytes)==ERROR_SUCCESS){
  const std::filesystem::path folder(directory.data());
  if(folder.is_absolute())if(auto module=LoadLibraryExW((folder/L"nvngx.dll").c_str(),nullptr,
                         LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32))return module;
 }
 return LoadLibraryExW(L"Z:\\run\\host\\usr\\lib\\nvidia\\wine\\nvngx.dll",nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
}
}
