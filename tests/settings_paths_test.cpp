#include "../src/providers/settings_paths.hpp"
#include <cassert>
int main(){
 using namespace mcd2::providers;
 const std::filesystem::path local="/fixture/Local";
 auto steam=settingsPaths(local,SettingsEdition::Steam,{});
 assert(steam&&steam->saves==local/"Dungeons2/Saved/SaveGames");
 assert(steam->authority==local/"Dungeons2/Saved/MCD2Graphics");
 assert(!settingsPaths(local,SettingsEdition::Unknown,{}));
 assert(!settingsPaths(local,SettingsEdition::WinGDK,{}));
 const auto gdk=local/"Dungeons2/Saved/MCD2Graphics/WinGDK"/std::string(64,'a')/"SaveGames";
 auto selected=settingsPaths(local,SettingsEdition::WinGDK,gdk);
 assert(selected&&selected->saves==gdk&&selected->authority!=steam->authority);
 assert(!settingsPaths(local,SettingsEdition::WinGDK,steam->saves));
 assert(!settingsPaths(local,SettingsEdition::WinGDK,gdk/"../SaveGames"));
 assert(!settingsPaths(local,SettingsEdition::WinGDK,local/"Dungeons2/Saved/MCD2Graphics/WinGDK/account/SaveGames"));
}
