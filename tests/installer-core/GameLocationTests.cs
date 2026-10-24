using MCD2.Installer;

static class GameLocationTests {
    public static void Run(Action<bool,string> check,Action<Action,string> throws,string temp) {
        var steam=Path.Combine(temp,"Locations/Steam");
        void Put(string root,string relative) {var p=Path.Combine(root,relative);Directory.CreateDirectory(Path.GetDirectoryName(p)!);File.WriteAllText(p,"location fixture");}
        Put(steam,InstallerEngine.Shipping);
        foreach(var relative in new[]{"","Dungeons","Dungeons/Binaries","Dungeons/Binaries/Win64",InstallerEngine.Shipping}) {
            var p=GameInstallationProfile.Select(Path.Combine(steam,relative),GameEnvironment.Windows);
            check(p.Root==Path.GetFullPath(steam)&&p.Edition==GameEdition.Steam,"Steam known folder/executable resolves to canonical root");
        }
        var wrapper=Path.Combine(temp,"Locations/XboxGame");var gdk=Path.Combine(wrapper,"Content");
        const string gdkExe="Dungeons/Binaries/WinGDK/Dungeons-WinGDK-Shipping.exe";
        Put(gdk,gdkExe);var prefix=Path.Combine(temp,"Locations/Xodus/pfx");Directory.CreateDirectory(prefix);
        foreach(var selection in new[]{wrapper,gdk,Path.Combine(gdk,"Dungeons"),Path.Combine(gdk,"Dungeons/Binaries"),Path.Combine(gdk,"Dungeons/Binaries/WinGDK"),Path.Combine(gdk,gdkExe)}) {
            var windows=GameInstallationProfile.Select(selection,GameEnvironment.Windows);
            check(windows.Root==Path.GetFullPath(gdk)&&windows.Edition==GameEdition.WinGDK,"Game Pass Content and known selections resolve to canonical root");
            var linux=GameInstallationProfile.Select(selection,GameEnvironment.Linux,managedPrefix:prefix);
            check(linux.Root==windows.Root&&linux.Launcher==GameLauncher.Xodus&&linux.Prefix==Path.GetFullPath(prefix),"Normalized Game Pass selection preserves explicit Xodus prefix");
        }
        throws(()=>GameInstallationProfile.Select(wrapper,GameEnvironment.Linux),"Content-wrapped Game Pass still requires managed prefix");
        Put(steam,gdkExe);
        throws(()=>GameInstallationProfile.Select(steam,GameEnvironment.Windows),"Mixed root remains ambiguous");
        check(GameInstallationProfile.Select(Path.Combine(steam,InstallerEngine.Shipping),GameEnvironment.Windows).Edition==GameEdition.Steam,"Steam executable resolves mixed root explicitly");
        check(GameInstallationProfile.Select(Path.Combine(steam,gdkExe),GameEnvironment.Windows).Edition==GameEdition.WinGDK,"GDK executable resolves mixed root explicitly");
        check(GameInstallationProfile.Select(Path.Combine(steam,"Dungeons/Binaries/WinGDK"),GameEnvironment.Windows).Edition==GameEdition.WinGDK,"GDK binary folder resolves mixed root explicitly");
        throws(()=>GameInstallationProfile.Select(Path.Combine(steam,InstallerEngine.Shipping),GameEnvironment.Windows,GameEdition.WinGDK),"Explicit edition cannot contradict selected executable");
        var unrelated=Path.Combine(steam,"unrelated");Directory.CreateDirectory(unrelated);
        throws(()=>GameInstallationProfile.Select(unrelated,GameEnvironment.Windows),"Unrelated child cannot select parent game");
        throws(()=>GameInstallationProfile.Select(Path.Combine(temp,"absent"),GameEnvironment.Windows),"Missing selection is refused");
        var text=Path.Combine(steam,"notes.txt");File.WriteAllText(text,"fixture");
        throws(()=>GameInstallationProfile.Select(text,GameEnvironment.Windows),"Non-executable selection is refused");
        var impostor=Path.Combine(steam,"Dungeons/Binaries/Win64/unrelated.exe");File.WriteAllText(impostor,"fixture");
        throws(()=>GameInstallationProfile.Select(impostor,GameEnvironment.Windows),"Unrelated executable cannot select adjacent game");
        var nested=Path.Combine(temp,"Locations/NoCrawl");Put(Path.Combine(nested,"nested"),InstallerEngine.Shipping);
        throws(()=>GameInstallationProfile.Select(nested,GameEnvironment.Windows),"Selection does not crawl arbitrary children");
        if(!OperatingSystem.IsWindows()) {
            var linked=Path.Combine(temp,"Locations/linked");Directory.CreateSymbolicLink(linked,gdk);
            throws(()=>GameInstallationProfile.Select(linked,GameEnvironment.Windows),"Linked root is refused");
            throws(()=>GameInstallationProfile.Select(Path.Combine(linked,gdkExe),GameEnvironment.Windows),"Linked ancestor of selected executable is refused");
            var linkedWrapper=Path.Combine(temp,"Locations/LinkedWrapper");Directory.CreateDirectory(linkedWrapper);Directory.CreateSymbolicLink(Path.Combine(linkedWrapper,"Content"),gdk);
            throws(()=>GameInstallationProfile.Select(linkedWrapper,GameEnvironment.Windows),"Linked Content wrapper is refused");
            var linkedExe=Path.Combine(temp,"Locations/Dungeons-Win64-Shipping.exe");File.CreateSymbolicLink(linkedExe,Path.Combine(steam,InstallerEngine.Shipping));
            throws(()=>GameInstallationProfile.Select(linkedExe,GameEnvironment.Windows),"Linked executable is refused");
        }
    }
}
