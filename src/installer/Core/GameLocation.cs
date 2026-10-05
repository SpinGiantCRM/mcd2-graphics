namespace MCD2.Installer;

public sealed partial class InstallerEngine {
    public const string SupportedEdition = "Steam Win64; Xbox / Game Pass (WinGDK) is not supported in this release";
    const string GdkFolder = "Dungeons/Binaries/WinGDK";

    static string LocateGame(string path) {
        var selected=Path.GetFullPath(path);
        if(File.Exists(selected)) {
            if(!Path.GetExtension(selected).Equals(".exe",StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("Choose the game's .exe or its installation folder.");
            if((File.GetAttributes(selected)&FileAttributes.ReparsePoint)!=0)
                throw new InvalidDataException("Choose the real game executable, not a symbolic link.");
            selected=Path.GetDirectoryName(selected)!;
        }
        if(!Directory.Exists(selected))throw new InvalidDataException("Game folder not found. Choose an existing installation folder or game executable.");
        // Only the selection, the known project/binary parents and Content.
        // Never crawl a drive or accidentally choose an unrelated parent game.
        var candidates=new List<string>{selected};
        var current=selected;
        var name=Path.GetFileName(current);
        if(name.Equals("Win64",StringComparison.OrdinalIgnoreCase) || name.Equals("WinGDK",StringComparison.OrdinalIgnoreCase))
            current=Path.GetDirectoryName(current)!;
        if(Path.GetFileName(current).Equals("Binaries",StringComparison.OrdinalIgnoreCase))
            current=Path.GetDirectoryName(current)!;
        if(Path.GetFileName(current).Equals("Dungeons",StringComparison.OrdinalIgnoreCase))
            candidates.Add(Path.GetDirectoryName(current)!);
        foreach(var root in candidates)
            if((File.GetAttributes(root)&FileAttributes.ReparsePoint)!=0)
                throw new InvalidDataException("Choose a real game folder, not a symbolic link.");
        var content=Path.Combine(selected,"Content");
        if(Directory.Exists(content))candidates.Add(content);
        foreach(var root in candidates)
            if(File.Exists(Target(root,Shipping)))return root;
        foreach(var root in candidates)
            if(Directory.Exists(Target(root,GdkFolder)))
                throw new InvalidDataException("Xbox / Game Pass (WinGDK) is not supported by this release. The installer requires the qualified Steam Win64 build.");
        throw new InvalidDataException("Game not found here. Choose the Steam installation folder, its Dungeons/Binaries/Win64 folder, or the game's .exe. Xbox / Game Pass (WinGDK) is not supported by this release.");
    }
}
