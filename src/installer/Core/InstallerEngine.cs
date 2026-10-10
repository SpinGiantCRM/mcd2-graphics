using System.Diagnostics;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace MCD2.Installer;

public record Check(string Name, string Version, string State, string Expected, string Found);
public record DependencyRelease(string Version, Dictionary<string,string> Files, string? ArchiveHash = null);
public record Dependency(string Id, string Name, string Version, string Url, Dictionary<string,string> Files, string? ArchiveHash = null, Dictionary<string,string>? ArchiveMembers = null, Dictionary<string,string>? UpgradeFromFiles = null, IReadOnlyList<Dictionary<string,string>>? UpgradeFromFileSets = null, string? MinimumVersion = null, string? CompatibilityNote = null, IReadOnlyList<DependencyRelease>? CompatibleReleases = null) {
    public string Requirement => MinimumVersion is {} minimum ? minimum+"+" : CompatibilityNote??Version;
    public IEnumerable<DependencyRelease> Releases => new[]{new DependencyRelease(Version,Files,ArchiveHash)}.Concat(CompatibleReleases??[]);
}
public record FrameworkBootstrap(bool HadFile, string? PreviousHash);
public record Receipt(string Version, Dictionary<string,string> Files, HdrBootstrap? Hdr = null, FrameworkBootstrap? Framework = null, Dictionary<string,string>? DependencyOverrides = null);

public sealed partial class InstallerEngine {
    public const string Shipping = "Dungeons/Binaries/Win64/Dungeons-Win64-Shipping.exe";
    public const string Marker = "Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json";
    public const string Runtime = "Dungeons/Binaries/Win64/MCD2Graphics/ngx-runtime/nvngx_dlss.dll";
    const long MaximumFile = 512 * 1024 * 1024;
    public const string FrameworkProxy = "Dungeons/Binaries/Win64/dxgi.dll";
    const string FrameworkRecovery = "Dungeons/Binaries/Win64/MCD2GraphicsDependencyBackups/";
    readonly string? previousFrameworkHash;
    readonly string gameHash;
    readonly string supportedBuild;
    readonly Func<Stream?> payload;
    readonly string? expandedPayloadRoot;
    readonly Dictionary<string,string> own;
    readonly Dictionary<string,Dictionary<string,string>> previousInstalls = new();
    readonly Dictionary<string,byte[]> selections = new();
    readonly HashSet<string> overrideEnabled = new();
    readonly Dictionary<string,string> overrideHashes = new();
    readonly Dictionary<string,string> overrideOriginals = new();
    public bool OverrideEnabled(string id)=>overrideEnabled.Contains(id);
    public void SetDependencyOverride(string id,bool enabled) {
        var d=Dependencies.Single(x=>x.Id==id);RequireClosed();
        if(enabled)overrideEnabled.Add(id);else overrideEnabled.Remove(id);
        foreach(var path in d.Files.Keys){selections.Remove(path);overrideHashes.Remove(path);overrideOriginals.Remove(path);}
    }
    static bool KnownDependencyHash(Dependency d,string path,string hash)=>d.Releases.Any(release=>release.Files.TryGetValue(path,out var pin)&&pin==hash) || d.UpgradeFromFiles!=null&&d.UpgradeFromFiles.TryGetValue(path,out var old)&&old==hash || d.UpgradeFromFileSets!=null&&d.UpgradeFromFileSets.Any(set=>set.TryGetValue(path,out var h)&&h==hash);
    DependencyRelease? RecognizedRelease(Dependency d) {
        if(GameRoot==null)return null;
        // Match a whole release, never a mixture of individually recognized files.
        var hashes=d.Files.Keys.ToDictionary(path=>path,path=>selections.TryGetValue(path,out var b)?Digest(b):File.Exists(Target(GameRoot,path))?DigestFile(Target(GameRoot,path)):null);
        return d.Releases.FirstOrDefault(release=>release.Files.All(f=>hashes[f.Key]==f.Value));
    }
    Dictionary<string,string> EffectiveFiles(Dependency d) {
        if(overrideEnabled.Contains(d.Id)&&d.Files.Keys.All(overrideHashes.ContainsKey))return d.Files.ToDictionary(x=>x.Key,x=>overrideHashes[x.Key]);
        return RecognizedRelease(d)?.Files??d.Files;
    }
    public void UseInstalledDependency(string id) {
        var d=Dependencies.Single(x=>x.Id==id);RequireClosed();
        if(!overrideEnabled.Contains(id))throw new InvalidDataException("Enable the untested-version override first.");
        var hashes=new Dictionary<string,string>();
        foreach(var path in d.Files.Keys){var file=Target(GameRoot??throw new InvalidDataException("Choose the game first."),path);if(!File.Exists(file)||new FileInfo(file).Length==0||new FileInfo(file).Length>MaximumFile)throw new InvalidDataException("Install every required dependency file first.");hashes[path]=DigestFile(file);}
        foreach(var pair in hashes){overrideHashes[pair.Key]=pair.Value;overrideOriginals[pair.Key]=pair.Value;selections.Remove(pair.Key);}
    }
    public string Version { get; }
    public string Description { get; }
    public IReadOnlyList<Dependency> Dependencies { get; }
    public string? GameRoot { get; private set; }
    public Action RequireClosed { get; set; } = EnsureGameClosed;
    public InstallerEngine(string dependencyJson, string manifestJson, Func<Stream?> payloadFactory) {
        payload = payloadFactory;
        using var l=JsonDocument.Parse(dependencyJson); using var m=JsonDocument.Parse(manifestJson);
        if(l.RootElement.GetProperty("dependencies").TryGetProperty("ReShade",out var reshade)&&reshade.TryGetProperty("bootstrapPreviousSHA256",out var original))previousFrameworkHash=original.GetString();
        gameHash=l.RootElement.GetProperty("game").GetProperty("exeSHA256").GetString()!;
        supportedBuild=l.RootElement.GetProperty("game").GetProperty("steamBuildID").ToString();
        Version=m.RootElement.GetProperty("version").GetString()!;
        Description=m.RootElement.TryGetProperty("description",out var description)?description.GetString()??throw new InvalidDataException("Invalid package description."):"Core graphics addon\nNative Video settings\nDLSS Super Resolution and DLAA\nFrame Generation preview (DLSS/DLAA + native HDR)\nHDR support\nReflex appears only when the rendering GPU and integration support it.";
        if(Description.Length is 0 or >1200)throw new InvalidDataException("Invalid package description.");
        own=ReadHashes(m.RootElement.GetProperty("files"));
        foreach(var p in own.Keys) ValidateRelative(p);
        if(m.RootElement.TryGetProperty("upgradeFrom",out var upgrades))foreach(var version in upgrades.EnumerateObject())previousInstalls.Add(version.Name,ReadHashes(version.Value));
        var deps=new List<Dependency>();
        foreach(var item in l.RootElement.GetProperty("dependencies").EnumerateObject()) {
            var d=item.Value;
            var files=d.TryGetProperty("files",out var f)?ReadHashes(f):new Dictionary<string,string> {{d.GetProperty("file").GetString()!,d.GetProperty("sha256").GetString()!}};
            foreach(var path in files.Keys) ValidateRelative(path);
            var url=d.TryGetProperty("download",out var u)?u.GetString():d.TryGetProperty("source",out u)?u.GetString():d.TryGetProperty("url",out u)?u.GetString():null;
            deps.Add(new(item.Name,item.Name switch {"BlueprintLoader"=>"Blueprint Loader", "ReShade"=>"ReShade full addon support", "RenoDXUEExtended"=>"RenoDX HDR support", "DLSSRuntime"=>"NVIDIA DLSS runtime", "Streamline"=>"NVIDIA Reflex / Frame Generation runtime", "FSRUpscaler"=>"AMD FSR upscaling runtime", "FSRFrameGeneration"=>"AMD Frame Generation runtime", _=>item.Name},
                d.TryGetProperty("version",out var v)?v.ToString():d.TryGetProperty("releaseTag",out v)?v.ToString():"Pinned", url??OfficialUrl(item.Name), files,
                d.TryGetProperty("archiveSHA256",out var a)?a.GetString():null, d.TryGetProperty("archiveMembers",out var members)?ReadHashes(members):null, d.TryGetProperty("upgradeFromFiles",out var prior)?ReadHashes(prior):null,
                d.TryGetProperty("upgradeFromFileSets",out var sets)?sets.EnumerateArray().Select(ReadHashes).ToList():null,
                d.TryGetProperty("minimumVersion",out var minimum)?minimum.GetString():null,
                d.TryGetProperty("compatibilityNote",out var note)?note.GetString():null,
                d.TryGetProperty("compatibleReleases",out var releases)?releases.EnumerateArray().Select(r=>new DependencyRelease(r.GetProperty("version").GetString()!,ReadHashes(r.GetProperty("files")),r.TryGetProperty("archiveSHA256",out var rh)?rh.GetString():null)).ToList():null));
            var dep=deps[^1];
            if(dep.MinimumVersion!=null&&(!System.Version.TryParse(dep.MinimumVersion,out var min)||!System.Version.TryParse(dep.Version,out var tested)||tested<min))throw new InvalidDataException("Invalid dependency minimum version.");
            foreach(var release in dep.CompatibleReleases??[]) {
                if(release.Files.Count!=dep.Files.Count||release.Files.Keys.Any(path=>!dep.Files.ContainsKey(path))||release.Files.Values.Any(h=>!Regex.IsMatch(h,"^[a-f0-9]{64}$")))throw new InvalidDataException("Invalid compatible dependency file set.");
                if(dep.MinimumVersion!=null&&(!System.Version.TryParse(release.Version,out var v2)||v2<System.Version.Parse(dep.MinimumVersion)))throw new InvalidDataException("Compatible dependency is below the API minimum.");
            }
        }
        Dependencies=deps;
    }
    // Normal packages contain visible, hash-pinned files beside the executable.
    public InstallerEngine(string dependencyJson, string manifestJson, string payloadDirectory)
        : this(dependencyJson,manifestJson,()=>null) {
        expandedPayloadRoot=Path.GetFullPath(payloadDirectory);
    }
    Dictionary<string,byte[]> ReadPayload() {
        var source=new Dictionary<string,byte[]>();
        if(expandedPayloadRoot!=null) {
            if(!Directory.Exists(expandedPayloadRoot))throw new InvalidDataException("Payload folder missing. Extract the complete installer ZIP before running it.");
            foreach(var item in own) {
                var path=Target(expandedPayloadRoot,item.Key);
                using var input=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.Read);
                if(input.Length>MaximumFile)throw new InvalidDataException("Package integrity failure.");
                using var memory=new MemoryStream();input.CopyTo(memory);
                var bytes=memory.ToArray();if(bytes.LongLength>MaximumFile||Digest(bytes)!=item.Value)throw new InvalidDataException("Package hash mismatch.");
                source.Add(item.Key,bytes);
            }
        } else {
            // Legacy stream callers remain supported; the GUI does not embed an archive.
            using var stream=payload()??throw new InvalidDataException("No qualified mod payload.");
            using var zip=new ZipArchive(stream,ZipArchiveMode.Read);
            foreach(var item in own) {
                var matches=zip.Entries.Where(x=>x.FullName==item.Key).ToList();if(matches.Count!=1||matches[0].Length>MaximumFile)throw new InvalidDataException("Package integrity failure.");
                using var input=matches[0].Open();using var memory=new MemoryStream();input.CopyTo(memory);var bytes=memory.ToArray();if(bytes.LongLength>MaximumFile||Digest(bytes)!=item.Value)throw new InvalidDataException("Package hash mismatch.");source.Add(item.Key,bytes);
            }
        }
        return source;
    }
    static Dictionary<string,string> ReadHashes(JsonElement e)=>e.EnumerateObject().ToDictionary(x=>x.Name,x=>x.Value.GetString()!);
    static string OfficialUrl(string id)=>id switch {"BlueprintLoader"=>"https://www.nexusmods.com/minecraftdungeons2/mods/2?tab=files", "ReShade"=>"https://reshade.me/", "RenoDXUEExtended"=>"https://github.com/marat569/renodx/releases/tag/nightly-20260928", "DLSSRuntime"=>"https://github.com/NVIDIA/DLSS", _=>throw new InvalidDataException("Unknown requirement")};
    public void SetGame(string path) {
        var root=Path.GetFullPath(path);
        if(!File.Exists(Target(root,Shipping)))throw new InvalidDataException("Select the Minecraft Dungeons II folder containing Dungeons.");
        if(GameRoot!=root){selections.Clear();overrideEnabled.Clear();overrideHashes.Clear();overrideOriginals.Clear();}
        GameRoot=root;
    }
    public string GameBuild() {
        if(GameRoot==null)return "Not selected";
        var steamapps=Directory.GetParent(GameRoot)?.Parent?.FullName;
        var acf=steamapps==null?null:Path.Combine(steamapps,"appmanifest_1912410.acf");
        if(acf==null||!File.Exists(acf))return "Unknown (executable fingerprint checked separately)";
        var match=Regex.Match(File.ReadAllText(acf),"\"buildid\"\\s+\"(\\d+)\"");
        return match.Success?match.Groups[1].Value:"Unknown";
    }
    public static IEnumerable<string> DetectGames(IEnumerable<string>? steamRoots=null) {
        var home=Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        var roots=steamRoots??new[] {Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),"Steam"),Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),"Steam"),
            Path.Combine(home,".steam/steam"),Path.Combine(home,".local/share/Steam"),Path.Combine(home,".var/app/com.valvesoftware.Steam/.local/share/Steam")};
        var libraries=new HashSet<string>(OperatingSystem.IsWindows()?StringComparer.OrdinalIgnoreCase:StringComparer.Ordinal);
        foreach(var root in roots) {
            if(string.IsNullOrWhiteSpace(root))continue;
            libraries.Add(root);var vdf=Path.Combine(root,"steamapps/libraryfolders.vdf");
            if(!File.Exists(vdf))continue;
            // Modern vdf has nested path keys; legacy vdf maps library numbers directly.
            foreach(Match x in Regex.Matches(File.ReadAllText(vdf),"\"(?:path|[0-9]+)\"\\s+\"([^\"]+)\"")) {
                var p=x.Groups[1].Value.Replace("\\\\","\\");if(Path.IsPathRooted(p))libraries.Add(p);
            }
        }
        foreach(var lib in libraries) {
            var p=Path.Combine(lib,"steamapps/common/Minecraft Dungeons II");if(File.Exists(Path.Combine(p,Shipping)))yield return Path.GetFullPath(p);
        }
    }
    public IReadOnlyList<Check> Scan() {
        if(GameRoot==null)return new[] {new Check("Minecraft Dungeons II",supportedBuild,"Choose game","","")};
        var result=new List<Check> {CheckFiles("Minecraft Dungeons II",supportedBuild,new(){{Shipping,gameHash}},false)};
        foreach(var d in Dependencies) {
            var check=CheckFiles(d.Name,RecognizedRelease(d)?.Version??d.Version,EffectiveFiles(d),true);
            if(overrideEnabled.Contains(d.Id)&&d.Files.Keys.All(overrideHashes.ContainsKey)&&check.State is "OK" or "Ready to install")check=check with {State="Untested override",Version="Unverified; tested: "+d.Version};
            result.Add(check);
        }
        var installed=File.Exists(Target(GameRoot,Marker));
        var payloadCheck=CheckFiles("Graphics mod payload",Version,own,false);
        string payloadState=installed?payloadCheck.State:"Not installed";
        if(installed) {
            try {
                var receipt=ReadReceipt(Target(GameRoot,Marker));ValidateOwnership(receipt);
                if(receipt.Version!=Version)payloadState="Upgrade available";
                else if(receipt.Hdr!=null)PrepareHdr(receipt.Hdr);
            } catch(InvalidDataException) {payloadState="Receipt or HDR startup setting needs attention";}
              catch(System.Text.Json.JsonException) {payloadState="Invalid install receipt";}
        }
        result.Add(payloadCheck with {State=payloadState});
        return result;
    }
    Check CheckFiles(string name,string version,Dictionary<string,string> files,bool staged) {
        var expected=new List<string>();var found=new List<string>();bool missing=false,wrong=false,pending=false;
        foreach(var pair in files) {
            expected.Add(pair.Value);
            var p=Target(GameRoot!,pair.Key);string? h=File.Exists(p)?DigestFile(p):null;
            if(h==pair.Value) {found.Add(h);continue;}
            if(staged && selections.TryGetValue(pair.Key,out var b)&&Digest(b)==pair.Value) {found.Add(pair.Value);pending=true;continue;}
            if(h==null) {missing=true;found.Add("Missing");}else {wrong=true;found.Add(h);}
        }
        return new(name,version,wrong?(name.StartsWith("ReShade")?"Wrong build/version — full addon support required":"Unsupported version"):missing?"Missing":pending?"Ready to install":"OK",string.Join(", ",expected),string.Join(", ",found));
    }
    public void SelectDependency(string id,string filename) {
        var d=Dependencies.Single(x=>x.Id==id);var experimental=overrideEnabled.Contains(id);
        if(id=="ReShade"&&d.Files.Keys.Any(x=>x.EndsWith("/dxgi.dll",StringComparison.Ordinal)))throw new InvalidDataException("Run the official full-addon installer for the shipping executable, then select Check again.");
        RequireClosed();if(new FileInfo(filename).Length>MaximumFile)throw new InvalidDataException("Download is too large.");var data=File.ReadAllBytes(filename);if(data.Length==0||data.Length>MaximumFile)throw new InvalidDataException("Download is too large.");
        var selected=new Dictionary<string,byte[]>();
        var archiveHash=Digest(data);
        var release=d.Releases.FirstOrDefault(r=>r.ArchiveHash==archiveHash);
        if(d.Files.Count==1&&(d.ArchiveHash==null||(experimental&&!(data.Length>=4&&data[0]==0x50&&data[1]==0x4b)))) {
            var f=d.Files.Single();if(!experimental&&!d.Releases.Any(r=>r.Files[f.Key]==archiveHash))throw WrongDownload(d,archiveHash);selected.Add(f.Key,data);
        } else {
            if(!experimental&&release==null)throw WrongDownload(d,archiveHash);
            using var zip=new ZipArchive(new MemoryStream(data),ZipArchiveMode.Read);
            long total=0;
            foreach(var e in zip.Entries) {
                ValidateRelative(e.FullName.TrimEnd('/'));
                if(e.Length>MaximumFile||(total+=e.Length)>2L*1024*1024*1024)throw new InvalidDataException("Archive exceeds extraction limits.");
                if((e.ExternalAttributes>>16&0xf000)==0xa000)throw new InvalidDataException("Archive symbolic links are not allowed.");
            }
            foreach(var f in d.Files) {
                var matches=zip.Entries.Where(x=>!x.FullName.EndsWith('/')&&(d.ArchiveMembers!=null?x.FullName==d.ArchiveMembers[f.Key]:Path.GetFileName(x.FullName)==Path.GetFileName(f.Key))).ToList();
                if(matches.Count!=1)throw new InvalidDataException("Archive must contain exactly one copy of each required file.");
                using var s=matches[0].Open();using var memory=new MemoryStream();s.CopyTo(memory);var b=memory.ToArray();if(b.Length==0||(!experimental&&Digest(b)!=release!.Files[f.Key]))throw WrongDownload(d,Digest(b));selected.Add(f.Key,b);
            }
        }
        foreach(var f in selected) {
            var target=Target(GameRoot??throw new InvalidDataException("Choose the game first."),f.Key);
            if(File.Exists(target)&&DigestFile(target)!=Digest(f.Value)&&DigestFile(target)!=d.Files[f.Key] && !(overrideOriginals.TryGetValue(f.Key,out var approved)&&DigestFile(target)==approved) && !KnownDependencyHash(d,f.Key,DigestFile(target)))throw new InvalidDataException("An existing dependency differs. Back it up and use its official upgrade workflow; the installer will not overwrite it.");
        }
        foreach(var f in selected){selections[f.Key]=f.Value;if(experimental)overrideHashes[f.Key]=Digest(f.Value);}
    }
    static Exception WrongDownload(Dependency d,string found)=>new InvalidDataException($"Unrecognized {d.Name} download. Requirement: {d.Requirement}; tested: {d.Version}. Found SHA-256: {found}. Choose a recognized official release, or use the explicit untested-version override.");
    public bool Ready=>GameRoot!=null&&Scan().Where(x=>x.Name!="Graphics mod payload").All(x=>x.State is "OK" or "Ready to install" or "Untested override");
    public void Install(bool repair=false) {
        RequireClosed();if(!Ready)throw new InvalidDataException("Resolve the requirements before installing.");
        var root=GameRoot!;var marker=Target(root,Marker);Receipt? old=null;
        if(File.Exists(marker)) {old=ReadReceipt(marker);if(!repair)throw new InvalidDataException("Already installed. Choose Repair / Verify.");ValidateOwnership(old);}
        var source=ReadPayload(); // Complete source validation before any game write.
        FrameworkBootstrap? framework=old?.Framework;
        byte[]? frameworkOriginal=null;
        if(own.ContainsKey(FrameworkProxy)&&previousFrameworkHash!=null) {
            if(framework!=null)ValidateFramework(framework);
            else {
                var existing=Target(root,FrameworkProxy);bool had=File.Exists(existing);
                if(had&&DigestFile(existing)!=previousFrameworkHash)throw new InvalidDataException("Unknown graphics bootstrap retained. Restore the supported ReShade version before upgrading.");
                framework=new(had,had?previousFrameworkHash:null);
                if(had)frameworkOriginal=File.ReadAllBytes(existing);
            }
        }
        // Validate every destination before the first write, including repair ownership.
        var modFiles=new Dictionary<string,string>(own);
        foreach(var dependency in Dependencies)foreach(var f in EffectiveFiles(dependency)) {
            var p=Target(root,f.Key);
            if(dependency.Id=="DLSSRuntime") {modFiles[f.Key]=f.Value;if(selections.TryGetValue(f.Key,out var b))source[f.Key]=b;else if(File.Exists(p)&&DigestFile(p)==f.Value)source[f.Key]=File.ReadAllBytes(p);}
            else if(selections.TryGetValue(f.Key,out var b)&&(!File.Exists(p)||DigestFile(p)!=f.Value))source[f.Key]=b;
        }
        foreach(var f in source) {
            var dest=Target(root,f.Key);if(!File.Exists(dest))continue;var h=DigestFile(dest);
            if(old!=null&&old.Files.TryGetValue(f.Key,out var previous)&&h==previous)continue;
            if(f.Key==FrameworkProxy&&frameworkOriginal!=null&&h==previousFrameworkHash)continue;
            if(!own.ContainsKey(f.Key)&&overrideOriginals.TryGetValue(f.Key,out var approvedHash)&&h==approvedHash)continue;
            if(!own.ContainsKey(f.Key)&&Dependencies.Any(x=>KnownDependencyHash(x,f.Key,h)))continue;
            throw new InvalidDataException("Existing or modified file retained. Resolve it before installation.");
        }
        var obsolete=old?.Files.Where(f=>!modFiles.ContainsKey(f.Key)).ToList()??[];
        foreach(var f in obsolete) {
            var path=Target(root,f.Key);
            if(File.Exists(path)&&DigestFile(path)!=f.Value)throw new InvalidDataException("Modified previous mod file retained. Repair was stopped before changing anything.");
        }
        var originals=new Dictionary<string,byte[]?>();var dirs=new HashSet<string>();
        try {
            if(frameworkOriginal!=null) {
                var recovery=Target(root,FrameworkRecovery+previousFrameworkHash+".bak");
                if(File.Exists(recovery)) {if(DigestFile(recovery)!=previousFrameworkHash)throw new InvalidDataException("Framework recovery file differs; no upgrade performed.");}
                else {originals[recovery]=null;var parent=Path.GetDirectoryName(recovery)!;RememberMissingDirs(parent,root,dirs);Directory.CreateDirectory(parent);AtomicWrite(recovery,frameworkOriginal);}
            }
            foreach(var f in source) {
                var dest=Target(root,f.Key);if(File.Exists(dest)&&DigestFile(dest)==Digest(f.Value))continue;
                originals[dest]=File.Exists(dest)?File.ReadAllBytes(dest):null;
                if(originals[dest]!=null&&!own.ContainsKey(f.Key)&&f.Key!=Runtime) {
                    var priorHash=Digest(originals[dest]!);var backup=Target(root,"Dungeons/Binaries/Win64/MCD2GraphicsDependencyBackups/"+priorHash+".bak");
                    if(File.Exists(backup)){if(DigestFile(backup)!=priorHash)throw new InvalidDataException("Dependency recovery file differs; upgrade stopped.");}
                    else{originals[backup]=null;var backupDir=Path.GetDirectoryName(backup)!;RememberMissingDirs(backupDir,root,dirs);Directory.CreateDirectory(backupDir);AtomicWrite(backup,originals[dest]!);}
                }
                var parent=Path.GetDirectoryName(dest)!;RememberMissingDirs(parent,root,dirs);Directory.CreateDirectory(parent);AtomicWrite(dest,f.Value);
            }
            foreach(var f in obsolete) {
                var path=Target(root,f.Key);if(!File.Exists(path))continue;
                originals[path]=File.ReadAllBytes(path);File.Delete(path);
            }
            originals[marker]=File.Exists(marker)?File.ReadAllBytes(marker):null;
            var bootstrap=PrepareHdr(old?.Hdr);
            if(bootstrap.Data!=null){var config=HdrPath();originals[config]=File.Exists(config)?File.ReadAllBytes(config):null;RememberMissingDirs(Path.GetDirectoryName(config)!,HdrRoot(),dirs);Directory.CreateDirectory(Path.GetDirectoryName(config)!);AtomicWrite(config,bootstrap.Data);}
            AtomicWrite(marker,Encoding.UTF8.GetBytes(JsonSerializer.Serialize(new Receipt(Version,modFiles,bootstrap.Record,framework,new Dictionary<string,string>(overrideHashes)),ReceiptOptions)));
        } catch {
            foreach(var f in originals.Reverse()) {if(f.Value==null)File.Delete(f.Key);else AtomicWrite(f.Key,f.Value);}
            foreach(var dir in dirs.OrderByDescending(x=>x.Length))if(Directory.Exists(dir)&&!Directory.EnumerateFileSystemEntries(dir).Any())Directory.Delete(dir);
            throw;
        }
    }
    public void Uninstall() {
        RequireClosed();var root=GameRoot??throw new InvalidDataException("Choose the game first.");var marker=Target(root,Marker);
        var receipt=ReadReceipt(marker);ValidateOwnership(receipt);
        foreach(var f in receipt.Files) {var p=Target(root,f.Key);if(File.Exists(p)&&DigestFile(p)!=f.Value)throw new InvalidDataException("Modified mod file retained. Uninstall was stopped before removing anything.");}
        byte[]? originalFramework=null;
        if(receipt.Framework is {HadFile:true} remembered) {ValidateFramework(remembered);originalFramework=File.ReadAllBytes(Target(root,FrameworkRecovery+remembered.PreviousHash+".bak"));}
        var config=RemoveHdr(receipt.Hdr); // Validate configuration before any mutation.
        if(config.Data!=null)AtomicWrite(HdrPath(receipt.Hdr!.ProjectOverride),config.Data);else if(config.Delete)File.Delete(HdrPath(receipt.Hdr!.ProjectOverride));
        // All checks completed. Only our bootstrap key is restored; other configuration and saves remain.
        foreach(var f in receipt.Files)File.Delete(Target(root,f.Key));
        if(originalFramework!=null)AtomicWrite(Target(root,FrameworkProxy),originalFramework);
        File.Delete(marker);
        foreach(var dir in receipt.Files.Keys.Select(x=>Path.GetDirectoryName(Target(root,x))!).Distinct().OrderByDescending(x=>x.Length)) {
            var p=dir;while(p!=root&&(Path.GetFileName(p) is "MCD2Graphics" or "ngx-runtime")) {if(!Directory.Exists(p)||Directory.EnumerateFileSystemEntries(p).Any())break;Directory.Delete(p);p=Path.GetDirectoryName(p)!;}
        }
    }
    void ValidateOwnership(Receipt r) {
        var expected=new Dictionary<string,string>(own){{Runtime,Dependencies.Single(x=>x.Id=="DLSSRuntime").Files[Runtime]}};
        if(r.Version!=Version && !previousInstalls.TryGetValue(r.Version,out expected))throw new InvalidDataException("Unknown installation version; no files were changed.");
        expected=new Dictionary<string,string>(expected!);
        if(r.Version==Version&&r.Files.TryGetValue(Runtime,out var runtimeHash)&&Dependencies.Single(x=>x.Id=="DLSSRuntime").Releases.Any(release=>release.Files[Runtime]==runtimeHash))expected[Runtime]=runtimeHash;
        if(r.DependencyOverrides!=null)foreach(var pair in r.DependencyOverrides) {
            if(own.ContainsKey(pair.Key)||!Dependencies.Any(d=>d.Files.ContainsKey(pair.Key))||!Regex.IsMatch(pair.Value,"^[a-f0-9]{64}$"))throw new InvalidDataException("Invalid dependency override receipt; files retained.");
            if(pair.Key==Runtime&&expected!.ContainsKey(Runtime))expected[Runtime]=pair.Value;
        }
        if(r.Files.Count!=expected!.Count||r.Files.Any(x=>!expected.TryGetValue(x.Key,out var h)||h!=x.Value))throw new InvalidDataException("Unexpected install receipt; no files were changed.");
        ValidateHdrRecord(r.Hdr);
        if(r.Version==Version&&own.ContainsKey(FrameworkProxy)&&previousFrameworkHash!=null&&r.Framework==null)throw new InvalidDataException("Missing graphics bootstrap ownership; files retained.");
        if(r.Framework!=null)ValidateFramework(r.Framework);
    }
    void ValidateFramework(FrameworkBootstrap f) {
        if(previousFrameworkHash==null||!own.ContainsKey(FrameworkProxy)||(!f.HadFile&&f.PreviousHash!=null)||(f.HadFile&&f.PreviousHash!=previousFrameworkHash))throw new InvalidDataException("Invalid graphics bootstrap ownership.");
        if(f.HadFile) {var backup=Target(GameRoot!,FrameworkRecovery+f.PreviousHash+".bak");if(!File.Exists(backup)||DigestFile(backup)!=f.PreviousHash)throw new InvalidDataException("Graphics bootstrap recovery is missing or modified; files retained.");}
    }
    static readonly JsonSerializerOptions ReceiptOptions=new(){PropertyNamingPolicy=JsonNamingPolicy.CamelCase,PropertyNameCaseInsensitive=true,WriteIndented=true};
    static Receipt ReadReceipt(string p)=>JsonSerializer.Deserialize<Receipt>(File.ReadAllText(p),ReceiptOptions)??throw new InvalidDataException("Invalid install receipt.");
    public string DiagnosticReport() {
        // Positive allowlist, never read logs, saves, tokens, configs or arbitrary error text.
        var report=new StringBuilder("MCD2 Graphics diagnostics\n");
        report.AppendLine("Mod: "+Version);report.AppendLine("Game build: "+GameBuild());report.AppendLine("Platform: "+(OperatingSystem.IsWindows()?"Windows ":"Linux ")+Environment.OSVersion.Version);
        foreach(var s in HardwareFacts())report.AppendLine(s);
        foreach(var c in Scan()) {report.AppendLine($"{c.Name}: {c.State} ({c.Version})");report.AppendLine("Expected SHA-256: "+c.Expected);report.AppendLine("Found SHA-256: "+c.Found);}
        report.AppendLine("No account data, private paths, saves or raw logs collected.");return report.ToString();
    }
    static IEnumerable<string> HardwareFacts() {
        string output="";
        try {output=OperatingSystem.IsWindows()?Run("powershell.exe","-NoProfile","-NonInteractive","-Command","Get-CimInstance Win32_VideoController | ForEach-Object { $_.Name + '|' + $_.DriverVersion }"):Run("nvidia-smi","--query-gpu=name,driver_version","--format=csv,noheader");}catch { }
        foreach(var line in output.Split('\n').Take(8)) {
            var value=line.Trim();if(value.Length>0&&value.Length<180&&Regex.IsMatch(value,"^[a-zA-Z0-9 .,_()|+-]+$"))yield return "GPU / driver: "+value;
        }
        if(string.IsNullOrWhiteSpace(output))yield return "GPU / driver: Unavailable";
    }
    static void RememberMissingDirs(string p,string root,HashSet<string> dirs) {while(p!=root&&!Directory.Exists(p)){dirs.Add(p);p=Path.GetDirectoryName(p)!;}}
    public static string DigestFile(string p) {using var s=File.OpenRead(p);return Convert.ToHexStringLower(SHA256.HashData(s));}
    public static string Digest(byte[] b)=>Convert.ToHexStringLower(SHA256.HashData(b));
    public static string Target(string root,string relative) {
        ValidateRelative(relative);root=Path.GetFullPath(root);var p=Path.GetFullPath(Path.Combine(root,relative.Replace('/',Path.DirectorySeparatorChar)));
        if(!p.StartsWith(root.TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar,OperatingSystem.IsWindows()?StringComparison.OrdinalIgnoreCase:StringComparison.Ordinal))throw new InvalidDataException("Unsafe destination.");
        // Reject reparse points and symbolic links at every existing component.
        var current=root;
        if((File.GetAttributes(root)&FileAttributes.ReparsePoint)!=0)throw new InvalidDataException("Choose a real game folder, not a symbolic link.");
        foreach(var part in relative.Split('/')) {current=Path.Combine(current,part);if((File.Exists(current)||Directory.Exists(current))&&(File.GetAttributes(current)&FileAttributes.ReparsePoint)!=0)throw new InvalidDataException("Linked destination refused.");}
        return p;
    }
    public static void ValidateRelative(string p) {if(string.IsNullOrWhiteSpace(p)||p.StartsWith('/')||p.Contains('\\')||p.Contains(':')||p.Split('/').Any(x=>x is "" or "." or ".."))throw new InvalidDataException("Unsafe package or archive path.");}
    static void AtomicWrite(string p,byte[] b) {var tmp=p+".mcd2-"+Guid.NewGuid().ToString("N")+".tmp";try{using(var stream=new FileStream(tmp,FileMode.CreateNew,FileAccess.Write)){stream.Write(b);stream.Flush(true);}if(DigestFile(tmp)!=Digest(b))throw new IOException("Copy verification failed.");File.Move(tmp,p,true);}finally{File.Delete(tmp);}}
    public static void EnsureGameClosed() {
        if(OperatingSystem.IsWindows()) {
            var output=Run("tasklist","/FI","IMAGENAME eq Dungeons-Win64-Shipping.exe","/FO","CSV","/NH");
            if(output.Split('\n').Any(x=>x.TrimStart().StartsWith("\"Dungeons-Win64-Shipping.exe\",",StringComparison.OrdinalIgnoreCase)))throw new InvalidOperationException("Close Minecraft Dungeons II before changing files.");
        } else foreach(var p in Directory.EnumerateDirectories("/proc").Where(x=>int.TryParse(Path.GetFileName(x),out _))) {
            byte[] b;try{b=File.ReadAllBytes(Path.Combine(p,"cmdline"));}catch(IOException){continue;}catch(UnauthorizedAccessException){continue;}
            var first=Encoding.UTF8.GetString(b.TakeWhile(x=>x!=0).ToArray());if(first.Contains("Dungeons-Win64-Shipping.exe",StringComparison.OrdinalIgnoreCase))throw new InvalidOperationException("Close Minecraft Dungeons II before changing files.");
        }
    }
    static string Run(string name,params string[] args) {
        var info=new ProcessStartInfo(name){UseShellExecute=false,RedirectStandardOutput=true,RedirectStandardError=true,CreateNoWindow=true};foreach(var a in args)info.ArgumentList.Add(a);
        using var p=Process.Start(info)??throw new IOException("System query unavailable.");var output=p.StandardOutput.ReadToEndAsync();var error=p.StandardError.ReadToEndAsync();if(!p.WaitForExit(10000)){p.Kill(true);throw new IOException("System query timed out.");}Task.WaitAll(output,error);if(p.ExitCode!=0)throw new IOException("System query failed.");return output.Result;
    }
}
