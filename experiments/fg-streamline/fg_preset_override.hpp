#pragma once
#include <NvApiDriverSettings.h>
// Called once before Streamline initialization. Linux changes only this process's
// DRS environment. Windows writes only an existing one-application game profile,
// with a flushed ownership receipt; never the base/global or a shared profile.
namespace mcd2::fg_preset {
inline int configure(const std::filesystem::path& root){
 const auto ini=root/L"MCD2Graphics"/L"DependencyOverrides.ini";
 const auto choice=GetPrivateProfileIntW(L"DLSS",L"FrameGenerationPreset",0,ini.c_str());if(choice>3)return -1;
 if(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"wine_get_version")){
  if(!choice)return 0;
  if(!SetEnvironmentVariableW(L"DXVK_NVAPI_DRS_NGX_DLSS_FG_OVERRIDE",L"on"))return -2;
  const wchar_t* preset=choice==3?L"render_preset_c":choice==2?L"render_preset_b":L"render_preset_a";
  return SetEnvironmentVariableW(L"DXVK_NVAPI_DRS_NGX_DLSS_FG_OVERRIDE_RENDER_PRESET_SELECTION",preset)?1:-2;
 }
 const wchar_t* section=L"DLSSFGDriverBackup";
 const bool pending=GetPrivateProfileIntW(section,L"Pending",0,ini.c_str())==1;
 // A/B have a public Streamline application setting; only C needs DRS.
 if(choice!=3&&!pending)return 0;
 auto module=LoadLibraryExW(L"nvapi64.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!module)return -10;
 struct Module{HMODULE h;~Module(){FreeLibrary(h);}} held{module};
 auto query=reinterpret_cast<void*(__cdecl*)(unsigned)>(GetProcAddress(module,"nvapi_QueryInterface"));if(!query)return -11;
 auto init=reinterpret_cast<decltype(&NvAPI_Initialize)>(query(0x0150e828));
 auto create=reinterpret_cast<decltype(&NvAPI_DRS_CreateSession)>(query(0x0694d52e));
 auto destroy=reinterpret_cast<decltype(&NvAPI_DRS_DestroySession)>(query(0xdad9cff8));
 auto load=reinterpret_cast<decltype(&NvAPI_DRS_LoadSettings)>(query(0x375dbd6b));
 auto save=reinterpret_cast<decltype(&NvAPI_DRS_SaveSettings)>(query(0xfcbc7e14));
 auto find=reinterpret_cast<decltype(&NvAPI_DRS_FindApplicationByName)>(query(0xeee566b2));
 auto profileInfo=reinterpret_cast<decltype(&NvAPI_DRS_GetProfileInfo)>(query(0x61cd6fd6));
 auto get=reinterpret_cast<decltype(&NvAPI_DRS_GetSetting)>(query(0x73bf8338));
 auto set=reinterpret_cast<decltype(&NvAPI_DRS_SetSetting)>(query(0x577dd202));
 auto erase=reinterpret_cast<decltype(&NvAPI_DRS_DeleteProfileSetting)>(query(0xe4a26362));
 if(!init||!create||!destroy||!load||!save||!find||!profileInfo||!get||!set||!erase||init()!=NVAPI_OK)return -11;
 NvDRSSessionHandle session=nullptr;if(create(&session)!=NVAPI_OK)return -12;
 struct Session{NvDRSSessionHandle h;decltype(destroy) end;~Session(){end(h);}} owned{session,destroy};
 if(load(session)!=NVAPI_OK)return -13;
 auto exe=std::make_unique<wchar_t[]>(2048);auto length=GetModuleFileNameW(nullptr,exe.get(),2048);if(!length||length>=2048)return -14;
 auto application=std::make_unique<NVDRS_APPLICATION>();application->version=NVDRS_APPLICATION_VER;
 NvDRSProfileHandle profile=nullptr;if(find(session,reinterpret_cast<NvU16*>(exe.get()),&profile,application.get())!=NVAPI_OK)return -20;
 auto info=std::make_unique<NVDRS_PROFILE>();info->version=NVDRS_PROFILE_VER;
 if(profileInfo(session,profile,info.get())!=NVAPI_OK||info->numOfApps!=1)return -21;
 auto priorProfile=std::make_unique<wchar_t[]>(2048);
 if(pending){GetPrivateProfileStringW(section,L"Profile",L"",priorProfile.get(),2048,ini.c_str());if(wcscmp(priorProfile.get(),reinterpret_cast<const wchar_t*>(info->profileName)))return -22;}
 const NvU32 ids[]={NGX_DLSS_FG_OVERRIDE_ID,NGX_DLSS_FG_OVERRIDE_RENDER_PRESET_SELECTION_ID};
 const wchar_t* presentKeys[]={L"EnablePresent",L"PresetPresent"};const wchar_t* valueKeys[]={L"EnableValue",L"PresetValue"};
 auto values=std::make_unique<NVDRS_SETTING[]>(2);bool stillOwned=true,originalUnchanged=true;
 for(unsigned i=0;i<2;++i){values[i].version=NVDRS_SETTING_VER;const auto result=get(session,profile,ids[i],&values[i]);
  if(result!=NVAPI_OK&&result!=NVAPI_SETTING_NOT_FOUND)return -23;
  if(result==NVAPI_OK&&values[i].settingType!=NVDRS_DWORD_TYPE)return -24;
  if(pending){stillOwned=stillOwned&&result==NVAPI_OK&&values[i].settingLocation==NVDRS_CURRENT_PROFILE_LOCATION&&!values[i].isCurrentPredefined&&values[i].u32CurrentValue==(i?3u:1u);
   const bool hadOriginal=GetPrivateProfileIntW(section,presentKeys[i],0,ini.c_str())==1;
   originalUnchanged=originalUnchanged&&(hadOriginal?(result==NVAPI_OK&&values[i].settingLocation==NVDRS_CURRENT_PROFILE_LOCATION&&!values[i].isCurrentPredefined&&values[i].u32CurrentValue==GetPrivateProfileIntW(section,valueKeys[i],0,ini.c_str())):(result==NVAPI_SETTING_NOT_FOUND||values[i].settingLocation!=NVDRS_CURRENT_PROFILE_LOCATION||values[i].isCurrentPredefined));}
  if(!pending){const bool present=result==NVAPI_OK&&values[i].settingLocation==NVDRS_CURRENT_PROFILE_LOCATION&&!values[i].isCurrentPredefined;
   if(!WritePrivateProfileStringW(section,presentKeys[i],present?L"1":L"0",ini.c_str())||
      !WritePrivateProfileStringW(section,valueKeys[i],std::to_wstring(values[i].u32CurrentValue).c_str(),ini.c_str()))return -26;}
 }
 if(pending&&!stillOwned&&!originalUnchanged)return -25;
 if(pending&&originalUnchanged&&choice!=3){WritePrivateProfileStringW(section,nullptr,nullptr,ini.c_str());return 0;}
 if(!pending){if(!WritePrivateProfileStringW(section,L"Profile",reinterpret_cast<const wchar_t*>(info->profileName),ini.c_str())||!WritePrivateProfileStringW(section,L"Pending",L"1",ini.c_str()))return -26;WritePrivateProfileStringW(nullptr,nullptr,nullptr,ini.c_str());}
 for(unsigned i=0;i<2;++i){NvAPI_Status result;
  if(choice==3){values[i].settingId=ids[i];values[i].settingType=NVDRS_DWORD_TYPE;values[i].u32CurrentValue=i?3u:1u;result=set(session,profile,&values[i]);}
  else if(GetPrivateProfileIntW(section,presentKeys[i],0,ini.c_str())){values[i].settingId=ids[i];values[i].u32CurrentValue=GetPrivateProfileIntW(section,valueKeys[i],0,ini.c_str());result=set(session,profile,&values[i]);}
  else result=erase(session,profile,ids[i]);
  if(result!=NVAPI_OK&&result!=NVAPI_SETTING_NOT_FOUND)return -27;
 }
 if(save(session)!=NVAPI_OK)return -28;
 if(choice!=3){WritePrivateProfileStringW(section,nullptr,nullptr,ini.c_str());WritePrivateProfileStringW(nullptr,nullptr,nullptr,ini.c_str());}
 return choice==3?1:0;
}
}
