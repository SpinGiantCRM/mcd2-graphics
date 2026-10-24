using MCD2.Installer;

static class ProfileTests {
    public static void Run(Action<bool,string> check, Action<Action,string> throws, string root) {
        var game=Path.Combine(root,"Profiles");
        Directory.CreateDirectory(Path.Combine(game,"Dungeons/Binaries/Win64"));
        File.WriteAllText(Path.Combine(game,InstallerEngine.Shipping),"fixture");
        var steamWindows=GameInstallationProfile.Select(game,GameEnvironment.Windows);
        check(steamWindows.Edition==GameEdition.Steam && steamWindows.ProcessName=="Dungeons-Win64-Shipping.exe","Steam Windows identity follows executable layout");
        check(steamWindows.MapDestination(InstallerEngine.Runtime)==InstallerEngine.Runtime,"Steam destinations remain unchanged");
        var steamLinux=GameInstallationProfile.Select(game,GameEnvironment.Linux);
        check(steamLinux.Launcher==GameLauncher.Steam && steamLinux.Prefix.EndsWith("compatdata/1912410/pfx"),"Steam Linux retains per-library prefix");
        var gdkExe=Path.Combine(game,"Dungeons/Binaries/WinGDK/Dungeons-WinGDK-Shipping.exe");
        Directory.CreateDirectory(Path.GetDirectoryName(gdkExe)!);File.WriteAllText(gdkExe,"fixture");
        throws(()=>GameInstallationProfile.Select(game,GameEnvironment.Windows),"Ambiguous edition requires explicit selection");
        var gdkWindows=GameInstallationProfile.Select(game,GameEnvironment.Windows,GameEdition.WinGDK);
        check(gdkWindows.ProcessName=="Dungeons-WinGDK-Shipping.exe" && gdkWindows.Launcher==GameLauncher.Xbox,"Game Pass Windows uses Xbox launch contract");
        check(gdkWindows.MapDestination(InstallerEngine.Marker)=="Dungeons/Binaries/WinGDK/MCD2Graphics/install-manifest.json","Game Pass receipt is isolated from Steam");
        check(gdkWindows.MapDestination("Dungeons/Content/Paks/~mods/MCD2Graphics_P.pak")=="Dungeons/Content/Paks/~mods/MCD2Graphics_P.pak","Shared content paths are not blindly rewritten");
        throws(()=>GameInstallationProfile.Select(game,GameEnvironment.Linux,GameEdition.WinGDK),"Game Pass Linux requires explicit managed prefix");
        var prefix=Path.Combine(root,"Xodus/pfx");Directory.CreateDirectory(prefix);
        var gdkLinux=GameInstallationProfile.Select(game,GameEnvironment.Linux,GameEdition.WinGDK,prefix);
        check(gdkLinux.Launcher==GameLauncher.Xodus && gdkLinux.Prefix==Path.GetFullPath(prefix),"Game Pass Linux preserves explicit Xodus prefix");
        check(gdkLinux.MapDestination(InstallerEngine.Runtime)=="Dungeons/Binaries/WinGDK/MCD2Graphics/ngx-runtime/nvngx_dlss.dll","Xodus maps provider runtime to Game Pass binary directory");
        check(gdkLinux.UserConfigRelative=="Dungeons2/Saved/Config/WinGDK","Game Pass config never uses Steam Windows folder");
        throws(()=>gdkLinux.MapDestination("../outside"),"Profile mapping preserves destination traversal refusal");
        var unknown=Path.Combine(root,"UnknownGame");Directory.CreateDirectory(unknown);
        throws(()=>GameInstallationProfile.Select(unknown,GameEnvironment.Windows),"Unknown executable layout is refused");
    }
}
