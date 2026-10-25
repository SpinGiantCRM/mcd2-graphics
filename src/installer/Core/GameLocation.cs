namespace MCD2.Installer;

// Resolve only the selected location, known game parents and one Content wrapper.
// Executable selection carries edition intent; unrelated files never select a game.
public sealed record GameLocation(string Root,GameEdition Edition) {
    public const string GdkShipping="Dungeons/Binaries/WinGDK/Dungeons-WinGDK-Shipping.exe";
    public static GameLocation Resolve(string selection,GameEdition? edition=null) {
        var selected=Path.TrimEndingDirectorySeparator(Path.GetFullPath(selection));
        if(!File.Exists(selected)&&!Directory.Exists(selected))
            throw new InvalidDataException("Choose an existing game folder or shipping executable.");
        RejectLinks(selected);
        string? executable=null;GameEdition? selectedEdition=null;
        if(File.Exists(selected)) {
            executable=selected;
            selectedEdition=Path.GetFileName(selected) switch {
                var n when n.Equals("Dungeons-Win64-Shipping.exe",StringComparison.OrdinalIgnoreCase)=>GameEdition.Steam,
                var n when n.Equals("Dungeons-WinGDK-Shipping.exe",StringComparison.OrdinalIgnoreCase)=>GameEdition.WinGDK,
                _=>throw new InvalidDataException("Choose Dungeons-Win64-Shipping.exe or Dungeons-WinGDK-Shipping.exe.")
            };
            selected=Path.GetDirectoryName(selected)!;
        }
        var candidates=new List<string>{selected};var current=selected;
        var leaf=Path.GetFileName(current);
        if((leaf.Equals("Win64",StringComparison.OrdinalIgnoreCase)||leaf.Equals("WinGDK",StringComparison.OrdinalIgnoreCase))&&
            Named(Path.GetDirectoryName(current),"Binaries")&&
            Named(Path.GetDirectoryName(Path.GetDirectoryName(current)),"Dungeons")) {
            selectedEdition??=leaf.Equals("Win64",StringComparison.OrdinalIgnoreCase)?GameEdition.Steam:GameEdition.WinGDK;
            current=Path.GetDirectoryName(current)!;
        }
        if(Path.GetFileName(current).Equals("Binaries",StringComparison.OrdinalIgnoreCase)&&
            Named(Path.GetDirectoryName(current),"Dungeons"))
            current=Path.GetDirectoryName(current)!;
        if(Path.GetFileName(current).Equals("Dungeons",StringComparison.OrdinalIgnoreCase))
            candidates.Add(Path.GetDirectoryName(current)!);
        if(edition!=null&&selectedEdition!=null&&edition!=selectedEdition)
            throw new InvalidDataException("The selected executable or binary folder belongs to a different game edition.");
        edition??=selectedEdition;
        candidates.Add(Path.Combine(selected,"Content"));
        foreach(var root in candidates.Distinct()) {
            if(!Directory.Exists(root))continue;
            RejectLinks(root);
            bool steam=File.Exists(InstallerEngine.Target(root,InstallerEngine.Shipping));
            bool gdk=File.Exists(InstallerEngine.Target(root,GdkShipping));
            if(!steam&&!gdk)continue;
            if(edition==null&&steam&&gdk)throw new InvalidDataException("Choose the game edition's shipping executable or binary folder.");
            var resolved=edition??(steam?GameEdition.Steam:GameEdition.WinGDK);
            if(resolved is not (GameEdition.Steam or GameEdition.WinGDK)||!(resolved==GameEdition.Steam?steam:gdk))
                throw new InvalidDataException("The selected edition's executable is missing.");
            var canonical=InstallerEngine.Target(root,resolved==GameEdition.Steam?InstallerEngine.Shipping:GdkShipping);
            if(executable!=null&&!canonical.Equals(executable,OperatingSystem.IsWindows()?StringComparison.OrdinalIgnoreCase:StringComparison.Ordinal))
                throw new InvalidDataException("Choose the shipping executable in the game's Dungeons/Binaries folder.");
            return new(root,resolved);
        }
        throw new InvalidDataException("Game not found here. Choose its installation folder, Dungeons/Binaries folder, or shipping executable.");
    }
    static bool Named(string? path,string value)=>string.Equals(Path.GetFileName(path),value,StringComparison.OrdinalIgnoreCase);
    static void RejectLinks(string path) {
        for(string? current=path;current!=null;current=Path.GetDirectoryName(current))
            if((File.GetAttributes(current)&FileAttributes.ReparsePoint)!=0)
                throw new InvalidDataException("Choose a real game location, not a symbolic link.");
    }
}
