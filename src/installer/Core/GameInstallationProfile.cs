namespace MCD2.Installer;

public enum GameEdition { Steam, WinGDK }
public enum GameEnvironment { Windows, Linux }
public enum GameLauncher { Steam, Xbox, Xodus }

// Edition comes from the selected executable layout, never from a launcher shortcut.
public sealed record GameInstallationProfile {
    public required string Root { get; init; }
    public required GameEdition Edition { get; init; }
    public required GameEnvironment Environment { get; init; }
    public required string Prefix { get; init; }
    public GameLauncher Launcher => Edition==GameEdition.Steam?GameLauncher.Steam:
        Environment==GameEnvironment.Windows?GameLauncher.Xbox:GameLauncher.Xodus;
    public string BinaryDirectory => "Dungeons/Binaries/"+(Edition==GameEdition.Steam?"Win64":"WinGDK");
    public string ProcessName => Edition==GameEdition.Steam?"Dungeons-Win64-Shipping.exe":"Dungeons-WinGDK-Shipping.exe";
    public string Shipping => BinaryDirectory+"/"+ProcessName;
    public string UserConfigRelative => "Dungeons2/Saved/Config/"+(Edition==GameEdition.Steam?"Windows":"WinGDK");
    public string MapDestination(string canonical) {
        InstallerEngine.ValidateRelative(canonical);
        const string steam="Dungeons/Binaries/Win64/";
        return canonical.StartsWith(steam,StringComparison.Ordinal)?BinaryDirectory+"/"+canonical[steam.Length..]:canonical;
    }
    public static GameInstallationProfile Select(string root,GameEnvironment environment,
        GameEdition? edition=null,string? managedPrefix=null) {
        var location=GameLocation.Resolve(root,edition);
        root=location.Root;edition=location.Edition;
        string prefix="";
        if(environment==GameEnvironment.Linux) {
            if(edition==GameEdition.WinGDK) {
                if(string.IsNullOrWhiteSpace(managedPrefix)||!Path.IsPathRooted(managedPrefix)||!Directory.Exists(managedPrefix))
                    throw new InvalidDataException("Select the existing Xodus-managed Wine prefix for this Game Pass installation.");
                prefix=Path.GetFullPath(managedPrefix);
            } else {
                var steamapps=Directory.GetParent(root)?.Parent?.FullName??throw new InvalidDataException("Steam library unavailable.");
                prefix=Path.Combine(steamapps,"compatdata/1912410/pfx");
            }
        }
        return new(){Root=root,Edition=edition.Value,Environment=environment,Prefix=prefix};
    }
}
