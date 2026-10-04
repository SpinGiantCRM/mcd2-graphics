// Read only the mod's two own slots; never inspect account or character saves.
#include "settings_bridge.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
static std::vector<uint8_t> read(const std::filesystem::path &path) {
 std::ifstream stream(path,std::ios::binary);
 return {(std::istreambuf_iterator<char>(stream)),{}};
}
int main(int argc,char **argv) {
 if(argc!=2)return 1;
 const std::filesystem::path directory(argv[1]);
 mcd2ui::Settings settings;mcd2ui::RuntimeState runtime;
 if(!mcd2ui::decode(read(directory/"MCD2GraphicsSettings.sav"),settings) ||
    !mcd2ui::decode_runtime(read(directory/"MCD2GraphicsRuntime.sav"),runtime))return 2;
 std::cout<<"{\"revision\":"<<settings.revision<<",\"mode\":"<<settings.mode
 <<",\"scale\":"<<settings.scale<<",\"preset\":"<<settings.preset
 <<",\"contextReady\":"<<settings.contextReady<<",\"sourceRevision\":"<<settings.sourceRevision
 <<",\"contextSessionMatches\":"<<(settings.contextSession==runtime.session?"true":"false")
 <<",\"sourceSessionMatches\":"<<(settings.sourceSession==runtime.session?"true":"false")
 <<",\"runtimeRevision\":"<<runtime.revision<<",\"phase\":"<<runtime.phase
 <<",\"error\":"<<runtime.error<<"}\n";
}
