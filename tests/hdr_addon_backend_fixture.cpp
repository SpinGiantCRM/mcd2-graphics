// Extracted production backend with synthetic ReShade cache and private files.
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "../src/providers/hdr_addon_activation.hpp"
using Key=mcd2::hdr_activation::Key;
std::filesystem::path advancedRoot;
Key cache;bool redirected=false;unsigned lastError=0;
constexpr unsigned ERROR_FILE_NOT_FOUND=2,FILE_ATTRIBUTE_REPARSE_POINT=1024;
unsigned GetEnvironmentVariableW(const wchar_t*,void*,unsigned){return redirected?1:0;}
unsigned GetModuleFileNameW(void*,wchar_t*out,unsigned size){auto s=(advancedRoot.parent_path()/"framework.asi").wstring();assert(s.size()+1<size);std::copy(s.begin(),s.end(),out);out[s.size()]=0;return unsigned(s.size());}
unsigned GetFileAttributesW(const auto*path){return std::filesystem::is_symlink(std::filesystem::path(path))?FILE_ATTRIBUTE_REPARSE_POINT:0;}
unsigned GetLastError(){return lastError;}
bool DeleteFileW(const auto*path){if(std::filesystem::remove(std::filesystem::path(path)))return true;lastError=ERROR_FILE_NOT_FOUND;return false;}
namespace reshade {
namespace internal {void*get_reshade_module_handle(){return nullptr;}}
bool get_config_value(void*,const char*,const char*,char*out,size_t*n){std::string b;for(const auto&s:cache.entries){b+=s;b+='\0';}if(!out){*n=b.empty()?0:b.size()+1;return !b.empty();}assert(*n>b.size());std::copy(b.begin(),b.end(),out);out[b.size()]=0;*n=b.size();return !b.empty();}
void set_config_value(void*,const char*,const char*,const char*value){assert(!value);cache={};}
void set_config_value(void*,const char*,const char*,const char*value,size_t size){cache={true,{}};size_t at=0;while(at<size){auto end=std::find(value+at,value+size,'\0');cache.entries.emplace_back(value+at,end);at=size_t(end-value)+1;}}
}
namespace mcd2::providers::store_detail {
bool discardPending(const std::filesystem::path&p){std::filesystem::remove(p);return true;}
bool writeFlushed(const std::filesystem::path&p,const auto&bytes){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());return f.good();}
bool replace(const std::filesystem::path&from,const std::filesystem::path&to){std::filesystem::remove(to);std::filesystem::rename(from,to);return true;}
}
// ACTUAL_BACKEND
int main(){
 using namespace mcd2::hdr_activation;advancedRoot=std::filesystem::current_path()/"MCD2Graphics";std::filesystem::create_directories(advancedRoot);
 const auto ini=advancedRoot.parent_path()/"ReShade.ini";const auto lease=advancedRoot/receiptName;
 HdrAddonBackend b;Activation a;auto r=a.apply(false,true,b);assert(!r.ready&&!r.conflict&&std::filesystem::exists(lease)&&disabled(cache));
 std::optional<Receipt>beforeFlush;assert(b.load(beforeFlush)&&beforeFlush&&beforeFlush->phase==Phase::Prepared);
 // Crash after cache write but before ReShade saves the file: the WAL remains
 // recoverable rather than falsely declaring an external ownership conflict.
 cache={};HdrAddonBackend afterCrash;Activation crashed;r=crashed.apply(false,true,afterCrash);assert(!r.ready&&!r.conflict&&disabled(cache));
 {std::ofstream f(ini);f<<"[ADDON]\nDisabledAddons="<<token<<"\n[Other]\nValue=keep\n";}
 r=a.apply(true,true,b);assert(!r.ready&&r.pendingRestart&&!cache.present&&std::filesystem::exists(lease));
 // The delayed config flush must not make the next request a false conflict.
 r=a.apply(true,true,b);assert(!r.ready&&!r.conflict&&std::filesystem::exists(lease));
 {std::ofstream f(ini);f<<"[Other]\nValue=keep\n";}
 HdrAddonBackend fresh;Activation restart;r=restart.apply(true,true,fresh);assert(r.ready&&!r.conflict&&!std::filesystem::exists(lease));
 {std::ofstream f(ini);f<<"[ADDON]\nDisabledAddons=\n";}cache={true,{}};auto&blank=b;Activation off;r=off.apply(false,true,blank);assert(!r.ready&&!r.conflict);std::optional<Receipt>owned;assert(blank.load(owned)&&owned&&owned->original.present&&owned->original.entries.empty());
 {std::ofstream f(ini);f<<"[ADDON]\nDisabledAddons="<<token<<",UserEdit\n";}auto oldCache=cache;r=off.apply(true,true,blank);assert(r.conflict&&cache==oldCache);assert(blank.load(owned)&&owned->phase==Phase::Yielded);
 {std::ofstream f(ini);f<<"[INSTALL]\nBasePath=elsewhere\n";}Key key;assert(!blank.read(key));redirected=true;std::filesystem::remove(ini);assert(!blank.read(key));redirected=false;
 std::filesystem::remove(lease);cache={true,{}};{std::ofstream f(ini);f<<"[ADDON]\nDisabledAddons\n";}HdrAddonBackend bare;assert(bare.read(key)&&key.present&&key.entries.empty());
}
