using System.Text;
namespace MCD2.Installer;
public record HdrBootstrap(bool HadFile, bool HadKey, string? PreviousValue, bool HadAAKey = false, string? PreviousAAValue = null, bool AARecorded = false, bool ProjectOverride = false);
public sealed partial class InstallerEngine {
    // Test override is never exposed in the normal installer UI. Actual roots are
    // derived from the selected Steam library or Windows LocalAppData.
    public string? ConfigRootOverride { get; set; }
    string HdrRoot(bool projectOverride=true) {
        if(ConfigRootOverride!=null)return Path.GetFullPath(ConfigRootOverride);
        if(projectOverride)return Path.Combine(GameRoot!,"Dungeons/Config");
        if(OperatingSystem.IsWindows())return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Dungeons2/Saved/Config/Windows");
        var steamapps=Directory.GetParent(GameRoot!)?.Parent?.FullName??throw new InvalidDataException("Steam library unavailable.");
        var user=Path.Combine(steamapps,"compatdata/1912410/pfx/drive_c/users/steamuser");
        if(!Directory.Exists(user))throw new InvalidDataException("Launch the game through Steam once to create its prefix, then install.");
        return Path.Combine(user,"AppData/Local/Dungeons2/Saved/Config/Windows");
    }
    string HdrPath(bool projectOverride=true){var root=HdrRoot(projectOverride);var cursor=root;while(!Directory.Exists(cursor)){cursor=Path.GetDirectoryName(cursor)??throw new InvalidDataException("Configuration folder unavailable.");}var relative=Path.GetRelativePath(cursor,root).Replace('\\','/');Target(cursor,(relative=="."?"":relative+"/")+(projectOverride?"UserEngine.ini":"Engine.ini"));return Path.Combine(root,projectOverride?"UserEngine.ini":"Engine.ini");}
    static void ValidateHdrRecord(HdrBootstrap? h){if(h!=null&&((h.AARecorded&&h.HadAAKey&&(h.PreviousAAValue==null||h.PreviousAAValue.Length>100||h.PreviousAAValue.Contains('\n')||h.PreviousAAValue.Contains('\r')))||(!h.HadAAKey&&h.PreviousAAValue!=null)))throw new InvalidDataException("Invalid temporal-AA ownership record.");if(h!=null&&((h.HadKey && (h.PreviousValue==null||h.PreviousValue.Length>100||h.PreviousValue.Contains('\n')||h.PreviousValue.Contains('\r')))||(!h.HadKey&&h.PreviousValue!=null)))throw new InvalidDataException("Invalid HDR ownership record.");}
    static (int Section,int End,int Key,string? Value) FindKey(List<string> lines, string keyName="r.AllowHDR"){int section=-1,end=lines.Count,key=-1;string? value=null;bool inside=false;
        for(int n=0;n<lines.Count;n++){var line=lines[n].Trim();if(line.StartsWith('[')&&line.EndsWith(']')){if(inside)end=n;inside=line.Equals("[SystemSettings]",StringComparison.OrdinalIgnoreCase);if(inside){if(section>=0)throw new InvalidDataException("Duplicate SystemSettings sections; configure HDR manually before installing.");section=n;end=lines.Count;}}
            if(inside&&line.StartsWith(keyName,StringComparison.OrdinalIgnoreCase)){var eq=line.IndexOf('=');if(eq<0||!line[..eq].Trim().Equals(keyName,StringComparison.OrdinalIgnoreCase))continue;if(key>=0)throw new InvalidDataException("Duplicate HDR keys; configure HDR manually before installing.");key=n;value=line[(eq+1)..].Trim();}}
        return(section,end,key,value);
    }
    static void SetKey(List<string> lines,string key,string value) {
        var f=FindKey(lines,key);
        if(f.Key>=0)lines[f.Key]=key+"="+value;
        else if(f.Section>=0)lines.Insert(f.End,key+"="+value);
        else{if(lines.Count>0&&lines[^1]!="")lines.Add("");lines.Add("[SystemSettings]");lines.Add(key+"="+value);}
    }
    (HdrBootstrap Record,byte[]? Data) PrepareHdr(HdrBootstrap? existing){
        ValidateHdrRecord(existing);if(existing!=null&&!existing.ProjectOverride)throw new InvalidDataException("Uninstall the earlier test candidate before installing this startup-configuration revision.");var path=HdrPath();bool had=File.Exists(path);
        if(had&&new FileInfo(path).Length>2*1024*1024)throw new InvalidDataException("Engine settings are too large; no files changed.");
        var lines=had?File.ReadAllLines(path).ToList():new List<string>();var hdr=FindKey(lines);var aa=FindKey(lines,"r.AntiAliasingMethod");
        if(existing!=null && hdr.Value!="1")throw new InvalidDataException("The HDR bootstrap was edited. Your setting is retained; restore r.AllowHDR=1 or uninstall the old mod before repair.");
        if(existing!=null && existing.AARecorded && aa.Value!="2")throw new InvalidDataException("The temporal-AA bootstrap was edited. Your setting is retained; restore r.AntiAliasingMethod=2 or uninstall the old mod before repair.");
        var record=existing??new HdrBootstrap(had,hdr.Key>=0,hdr.Value,ProjectOverride:true);
        if(!record.AARecorded)record=record with {HadAAKey=aa.Key>=0,PreviousAAValue=aa.Value,AARecorded=true};
        if(hdr.Value=="1" && aa.Value=="2")return(record,null);
        // SR replaces the shipped temporal-AA shader. A default TSR path cannot
        // provide that boundary; configure the required base and own only these keys.
        SetKey(lines,"r.AllowHDR","1");SetKey(lines,"r.AntiAliasingMethod","2");
        return(record,Encoding.UTF8.GetBytes(string.Join(Environment.NewLine,lines)+Environment.NewLine));
    }
    static bool RestoreKey(List<string> lines,string key,string owned,bool had,string? previous){
        var f=FindKey(lines,key);if(f.Value!=owned)return false; // Retain user edits.
        if(had)lines[f.Key]=key+"="+previous;
        else{lines.RemoveAt(f.Key);if(f.Section>=0){var next=f.Section+1;while(next<lines.Count&&string.IsNullOrWhiteSpace(lines[next]))next++;if(next==lines.Count||lines[next].TrimStart().StartsWith('['))lines.RemoveAt(f.Section);}}
        return true;
    }
    (byte[]? Data,bool Delete) RemoveHdr(HdrBootstrap? record){
        if(record==null)return(null,false);ValidateHdrRecord(record);var path=HdrPath(record.ProjectOverride);if(!File.Exists(path))return(null,false);
        var lines=File.ReadAllLines(path).ToList();bool changed=RestoreKey(lines,"r.AllowHDR","1",record.HadKey,record.PreviousValue);
        if(record.AARecorded)changed=RestoreKey(lines,"r.AntiAliasingMethod","2",record.HadAAKey,record.PreviousAAValue)||changed;
        if(!changed)return(null,false);
        if(!record.HadFile&&lines.All(string.IsNullOrWhiteSpace))return(null,true);
        return(Encoding.UTF8.GetBytes(string.Join(Environment.NewLine,lines)+Environment.NewLine),false);
    }
}
