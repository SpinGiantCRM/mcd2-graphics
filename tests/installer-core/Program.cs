using MCD2.Installer;
using System.IO.Compression;
using System.Text;
using System.Text.Json;
int cases=0;
void Check(bool b,string name){if(!b)throw new Exception(name);cases++;Console.WriteLine("PASS "+name);}
void Throws(Action a,string name){bool caught=false;try{a();}catch(Exception e)when(e is InvalidDataException or InvalidOperationException or IOException){caught=true;}Check(caught,name);}
var temp=Path.Combine(Path.GetTempPath(),"mcd2-installer-test-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(temp);
try {
 byte[] file=Encoding.UTF8.GetBytes("qualified test payload");string hash=InstallerEngine.Digest(file);var own=new Dictionary<string,string>{{"Dungeons/Binaries/Win64/mcd2-graphics.addon64",hash}};
 var dependencies=new Dictionary<string,object>{["ReShade"]=new{version="test",file="Dungeons/Binaries/Win64/dxgi.dll",sha256=hash,url="https://reshade.me/"},["RenoDXUEExtended"]=new{version="test",file="Dungeons/Binaries/Win64/renodx-ue-extended.addon64",sha256=hash,url="https://github.com/marat569/renodx"},["DLSSRuntime"]=new{version="test",file=InstallerEngine.Runtime,sha256=hash,url="https://github.com/NVIDIA/DLSS"}};
 string lockJson=JsonSerializer.Serialize(new{game=new{steamBuildID="123",exeSHA256=hash},dependencies});
 var manifest=JsonSerializer.Serialize(new{version="candidate.test",files=own});
 byte[] Payload(){using var memory=new MemoryStream();using(var z=new ZipArchive(memory,ZipArchiveMode.Create,true)){using var s=z.CreateEntry(own.Keys.Single()).Open();s.Write(file);}return memory.ToArray();}
 InstallerEngine Engine(Func<Stream?>? payload=null){var e=new InstallerEngine(lockJson,manifest,payload??(()=>new MemoryStream(Payload())));e.RequireClosed=()=>{};e.ConfigRootOverride=Path.Combine(temp,"Config");e.SetGame(temp);return e;}
 void Put(string path,byte[]? bytes=null){var p=Path.Combine(temp,path);Directory.CreateDirectory(Path.GetDirectoryName(p)!);File.WriteAllBytes(p,bytes??file);}
 Put(InstallerEngine.Shipping);Put("Dungeons/Binaries/Win64/dxgi.dll");Put("Dungeons/Binaries/Win64/renodx-ue-extended.addon64");
 var runtime=Path.Combine(temp,"download.dll");File.WriteAllBytes(runtime,file);
 var e=Engine();Check(!e.Ready,"Missing runtime prevents installation");e.SelectDependency("DLSSRuntime",runtime);Check(e.Ready,"Selected hash verified runtime makes install ready");
 e.Install();Check(InstallerEngine.DigestFile(Path.Combine(temp,own.Keys.Single()))==hash,"Payload installed and hashed");
 Check(File.ReadAllText(Path.Combine(temp,InstallerEngine.Marker)).Contains("candidate.test"),"Candidate version recorded");
 Throws(()=>e.Install(),"Duplicate install refused");
 Put(own.Keys.Single(),Encoding.UTF8.GetBytes("modified"));Throws(()=>e.Uninstall(),"Modified file stops uninstall before mutation");Throws(()=>e.Install(true),"Modified file stops repair");Check(File.Exists(Path.Combine(temp,InstallerEngine.Runtime)),"Runtime retained after failed uninstall");
 Put(own.Keys.Single());File.Delete(Path.Combine(temp,own.Keys.Single()));e.Install(true);Check(File.Exists(Path.Combine(temp,own.Keys.Single())),"Missing owned file repaired");
 var report=e.DiagnosticReport();Check(!report.Contains(temp)&&!report.Contains(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile)),"Report excludes private paths");
 e.Uninstall();Check(!File.Exists(Path.Combine(temp,own.Keys.Single()))&&!File.Exists(Path.Combine(temp,InstallerEngine.Runtime)),"Own payload and runtime removed");Check(File.Exists(Path.Combine(temp,"Dungeons/Binaries/Win64/dxgi.dll")),"External dependency preserved");
 Throws(()=>InstallerEngine.Target(temp,"../outside"),"Traversal rejected");Throws(()=>InstallerEngine.Target(temp,"C:/absolute"),"Drive path rejected");Throws(()=>InstallerEngine.Target(temp,"a\\b"),"Backslash archive path rejected");
 var evil=Engine(()=>new MemoryStream(Encoding.UTF8.GetBytes("broken zip")));evil.SelectDependency("DLSSRuntime",runtime);Throws(()=>evil.Install(),"Broken package refused before writing");Check(!File.Exists(Path.Combine(temp,InstallerEngine.Marker)),"Failed install leaves no receipt");
 var blocked=Engine();blocked.RequireClosed=()=>throw new InvalidOperationException("Running game");Throws(()=>blocked.SelectDependency("DLSSRuntime",runtime),"Running game blocks dependency selection");
 // HDR bootstrap preserves unrelated lines and restores just its owned key.
 var config=Path.Combine(temp,"Config/UserEngine.ini");Directory.CreateDirectory(Path.GetDirectoryName(config)!);File.WriteAllText(config,"[SystemSettings]\nOther=keep\nr.AllowHDR=0\n[Elsewhere]\nValue=keep\n");
 var hdr=Engine();hdr.SelectDependency("DLSSRuntime",runtime);hdr.Install();Check(File.ReadAllText(config).Contains("r.AllowHDR=1")&&File.ReadAllText(config).Contains("Other=keep"),"HDR bootstrap changes only owned key");hdr.Uninstall();Check(File.ReadAllText(config).Contains("r.AllowHDR=0")&&File.ReadAllText(config).Contains("Value=keep"),"HDR uninstall restores previous value");
 hdr=Engine();hdr.SelectDependency("DLSSRuntime",runtime);hdr.Install();File.WriteAllText(config,File.ReadAllText(config).Replace("r.AllowHDR=1","r.AllowHDR=0"));Check(hdr.Scan().Single(x=>x.Name=="Graphics mod payload").State!="OK","Verify detects edited HDR bootstrap");Throws(()=>hdr.Install(true),"Edited HDR bootstrap blocks repair without overwrite");hdr.Uninstall();Check(File.ReadAllText(config).Contains("r.AllowHDR=0"),"Uninstall retains edited HDR setting");
 File.WriteAllText(config,"[SystemSettings]\nr.AllowHDR=0\nr.AllowHDR=1\n");hdr=Engine();hdr.SelectDependency("DLSSRuntime",runtime);Throws(()=>hdr.Install(),"Duplicate HDR key refused");Check(!File.Exists(Path.Combine(temp,own.Keys.Single()))&&!File.Exists(Path.Combine(temp,InstallerEngine.Marker)),"Configuration failure rolls back payload and receipt");File.Delete(config);
 // Fresh/default renderer configuration must supply the TAA boundary for SR.
 File.WriteAllText(config,"[SystemSettings]\nr.AntiAliasingMethod=4\nOther=keep\n");hdr=Engine();hdr.SelectDependency("DLSSRuntime",runtime);hdr.Install();Check(File.ReadAllText(config).Contains("r.AntiAliasingMethod=2")&&File.ReadAllText(config).Contains("Other=keep"),"Temporal AA bootstrap preserves unrelated settings");hdr.Uninstall();Check(File.ReadAllText(config).Contains("r.AntiAliasingMethod=4")&&!File.ReadAllText(config).Contains("r.AllowHDR"),"Uninstall restores previous AA and removes added HDR");
 hdr=Engine();hdr.SelectDependency("DLSSRuntime",runtime);hdr.Install();File.WriteAllText(config,File.ReadAllText(config).Replace("r.AntiAliasingMethod=2","r.AntiAliasingMethod=3"));Throws(()=>hdr.Install(true),"Edited AA bootstrap blocks repair");hdr.Uninstall();Check(File.ReadAllText(config).Contains("r.AntiAliasingMethod=3"),"Uninstall retains edited AA");File.Delete(config);
 // Steam discovery includes additional modern and legacy VDF libraries.
 var steam=Path.Combine(temp,"Steam");Directory.CreateDirectory(Path.Combine(steam,"steamapps"));var lib=Path.Combine(temp,"Library");var game=Path.GetFullPath(Path.Combine(lib,"steamapps/common/Minecraft Dungeons II"));Directory.CreateDirectory(Path.GetDirectoryName(Path.Combine(game,InstallerEngine.Shipping))!);File.WriteAllBytes(Path.Combine(game,InstallerEngine.Shipping),file);File.WriteAllText(Path.Combine(steam,"steamapps/libraryfolders.vdf"),"\"libraryfolders\" { \"1\" { \"path\" \""+lib.Replace("\\","\\\\")+"\" } }");Check(InstallerEngine.DetectGames(new[]{steam}).Contains(game),"Modern Steam library detected");File.WriteAllText(Path.Combine(steam,"steamapps/libraryfolders.vdf"),"\"1\" \""+lib.Replace("\\","\\\\")+"\"");Check(InstallerEngine.DetectGames(new[]{steam}).Contains(game),"Legacy Steam library detected");
 // Known dependency upgrades require exact previous hashes and retain recovery bytes.
 var oldBytes=Encoding.UTF8.GetBytes("qualified previous dependency");var oldHash=InstallerEngine.Digest(oldBytes);var deps2=new Dictionary<string,object>(dependencies);string pathA="Dungeons/Content/Paks/~mods/BlueprintLoader/a.pak",pathB="Dungeons/Content/Paks/~mods/BlueprintLoader/b.utoc";
 byte[] MakeZip(params string[] names){using var m=new MemoryStream();using(var z=new ZipArchive(m,ZipArchiveMode.Create,true))foreach(var name in names){using var entry=z.CreateEntry(name).Open();entry.Write(file);}return m.ToArray();}
 var archive=MakeZip("BlueprintLoader/a.pak","BlueprintLoader/b.utoc");deps2["BlueprintLoader"]=new{version="2.0",files=new Dictionary<string,string>{{pathA,hash},{pathB,hash}},upgradeFromFiles=new Dictionary<string,string>{{pathA,oldHash},{pathB,oldHash}},archiveSHA256=InstallerEngine.Digest(archive),url="https://www.nexusmods.com/minecraftdungeons2/mods/2"};
 var lock2=JsonSerializer.Serialize(new{game=new{steamBuildID="123",exeSHA256=hash},dependencies=deps2});var zipFile=Path.Combine(temp,"loader.zip");File.WriteAllBytes(zipFile,archive);Put(pathA,oldBytes);Put(pathB,oldBytes);
 var upgrade=new InstallerEngine(lock2,manifest,()=>new MemoryStream(Payload())){RequireClosed=()=>{},ConfigRootOverride=Path.Combine(temp,"Config")};upgrade.SetGame(temp);upgrade.SelectDependency("DLSSRuntime",runtime);upgrade.SelectDependency("BlueprintLoader",zipFile);upgrade.Install();Check(InstallerEngine.DigestFile(Path.Combine(temp,pathA))==hash,"Known Blueprint dependency upgraded");Check(InstallerEngine.DigestFile(Path.Combine(temp,"Dungeons/Binaries/Win64/MCD2GraphicsDependencyBackups/"+oldHash+".bak"))==oldHash,"Dependency recovery hash retained");upgrade.Uninstall();Check(File.Exists(Path.Combine(temp,pathA)),"Upgraded external dependency retained on uninstall");
 Put(pathA,Encoding.UTF8.GetBytes("unknown change"));Throws(()=>upgrade.SelectDependency("BlueprintLoader",zipFile),"Unknown dependency not overwritten");
 var unsafeZip=MakeZip("../a.pak","b.utoc");deps2["BlueprintLoader"]=new{version="2.0",files=new Dictionary<string,string>{{pathA,hash},{pathB,hash}},archiveSHA256=InstallerEngine.Digest(unsafeZip),url="https://www.nexusmods.com/minecraftdungeons2/mods/2"};var unsafeLock=JsonSerializer.Serialize(new{game=new{steamBuildID="123",exeSHA256=hash},dependencies=deps2});var unsafeEngine=new InstallerEngine(unsafeLock,manifest,()=>new MemoryStream(Payload())){RequireClosed=()=>{}};unsafeEngine.SetGame(temp);File.WriteAllBytes(zipFile,unsafeZip);Throws(()=>unsafeEngine.SelectDependency("BlueprintLoader",zipFile),"Archive traversal refused before extraction");
 // Older qualified receipts upgrade through repair; arbitrary receipt hashes do not grant ownership.
 var oldManifest=new Dictionary<string,string>(own){{InstallerEngine.Runtime,hash}};Put(own.Keys.Single());Put(InstallerEngine.Runtime);Put(InstallerEngine.Marker,Encoding.UTF8.GetBytes(JsonSerializer.Serialize(new{version="previous.test",files=oldManifest})));var withOld=JsonSerializer.Serialize(new{version="candidate.test",files=own,upgradeFrom=new Dictionary<string,Dictionary<string,string>>{{"previous.test",oldManifest}}});var migration=new InstallerEngine(lockJson,withOld,()=>new MemoryStream(Payload())){RequireClosed=()=>{},ConfigRootOverride=Path.Combine(temp,"Config")};migration.SetGame(temp);migration.Install(true);Check(File.ReadAllText(Path.Combine(temp,InstallerEngine.Marker)).Contains("candidate.test"),"Qualified previous receipt migrated");migration.Uninstall();Put(InstallerEngine.Marker,Encoding.UTF8.GetBytes(JsonSerializer.Serialize(new{version="candidate.test",files=new Dictionary<string,string>{{own.Keys.Single(),oldHash},{InstallerEngine.Runtime,hash}}})));Throws(()=>migration.Uninstall(),"Unqualified receipt hash does not grant ownership");Check(migration.Scan().Single(x=>x.Name=="Graphics mod payload").State!="OK","Verify rejects unqualified receipt");

 // Retired owned files are validated, then removed transactionally on repair.
 var retired="Dungeons/Binaries/Win64/MCD2Graphics/streamline/retired.dll";
 var retirementFiles=new Dictionary<string,string>(oldManifest){{retired,oldHash}};
 var retirementManifest=JsonSerializer.Serialize(new{version="candidate.test",files=own,upgradeFrom=new Dictionary<string,Dictionary<string,string>>{{"previous.test",retirementFiles}}});
 Put(own.Keys.Single());Put(InstallerEngine.Runtime);Put(retired,oldBytes);
 var retirementReceipt=Encoding.UTF8.GetBytes(JsonSerializer.Serialize(new{version="previous.test",files=retirementFiles}));Put(InstallerEngine.Marker,retirementReceipt);
 var retireEngine=new InstallerEngine(lockJson,retirementManifest,()=>new MemoryStream(Payload())){RequireClosed=()=>{},ConfigRootOverride=Path.Combine(temp,"Config")};retireEngine.SetGame(temp);
 Put(retired,Encoding.UTF8.GetBytes("modified retired file"));Throws(()=>retireEngine.Install(true),"Modified retired owned file blocks entire upgrade");Check(File.ReadAllBytes(InstallerEngine.Target(temp,InstallerEngine.Marker)).SequenceEqual(retirementReceipt),"Retirement preflight preserves old receipt");
 Put(retired,oldBytes);retireEngine.Install(true);Check(!File.Exists(InstallerEngine.Target(temp,retired)),"Qualified retired owned file removed on upgrade");retireEngine.Uninstall();
 // Expanded deployment: source validation completes before any game mutation.
 foreach(var ownedPath in new[]{InstallerEngine.Marker,own.Keys.Single(),InstallerEngine.Runtime}){var existing=InstallerEngine.Target(temp,ownedPath);if(File.Exists(existing))File.Delete(existing);}
 var payloadRoot=Path.Combine(temp,"ExtractedInstaller/payload");var sourceFile=Path.Combine(payloadRoot,own.Keys.Single());Directory.CreateDirectory(Path.GetDirectoryName(sourceFile)!);File.WriteAllBytes(sourceFile,file);
 InstallerEngine FolderEngine(string? directory=null,string? definition=null){var x=new InstallerEngine(lockJson,definition??manifest,directory??payloadRoot){RequireClosed=()=>{},ConfigRootOverride=Path.Combine(temp,"Config")};x.SetGame(temp);x.SelectDependency("DLSSRuntime",runtime);return x;}
 var folder=FolderEngine();folder.Install();Check(File.Exists(InstallerEngine.Target(temp,own.Keys.Single())),"Expanded payload installed with matching hash");folder.Uninstall();Check(!File.Exists(InstallerEngine.Target(temp,own.Keys.Single())),"Expanded payload uninstalled; dependencies retained");
 File.WriteAllText(sourceFile,"corrupt");Throws(()=>FolderEngine().Install(),"Expanded corrupt source rejected");Check(!File.Exists(InstallerEngine.Target(temp,own.Keys.Single()))&&!File.Exists(InstallerEngine.Target(temp,InstallerEngine.Marker)),"Corrupt expanded package writes no game files");
 File.Delete(sourceFile);Throws(()=>FolderEngine().Install(),"Expanded missing file rejected");Throws(()=>FolderEngine(Path.Combine(temp,"missing-payload")).Install(),"Missing payload folder rejected");
 using(var sparse=new FileStream(sourceFile,FileMode.Create))sparse.SetLength(512L*1024*1024+1);Throws(()=>FolderEngine().Install(),"Oversized expanded source rejected before reading");File.WriteAllBytes(sourceFile,file);
 var second="Dungeons/Binaries/Win64/second-owned.addon64";var two=JsonSerializer.Serialize(new{version="candidate.test",files=new Dictionary<string,string>(own){{second,hash}}});var secondSource=Path.Combine(payloadRoot,second);File.WriteAllText(secondSource,"corrupt");Throws(()=>FolderEngine(definition:two).Install(),"Later invalid expanded file rejects entire payload");Check(!File.Exists(InstallerEngine.Target(temp,own.Keys.Single()))&&!File.Exists(InstallerEngine.Target(temp,second)),"Expanded preflight prevents partial install");File.Delete(secondSource);
 var runningFolder=FolderEngine();runningFolder.RequireClosed=()=>throw new InvalidOperationException("Running game");Throws(()=>runningFolder.Install(),"Running game blocks expanded install");
 if(!OperatingSystem.IsWindows()) {File.Delete(sourceFile);File.CreateSymbolicLink(sourceFile,runtime);Throws(()=>FolderEngine().Install(),"Linked expanded source rejected");File.Delete(sourceFile);File.WriteAllBytes(sourceFile,file);}


 // FG bootstrap ownership: only a pinned former framework may be replaced.
 foreach(var path in new[]{InstallerEngine.Marker,InstallerEngine.Runtime,InstallerEngine.FrameworkProxy}){var f=InstallerEngine.Target(temp,path);if(File.Exists(f))File.Delete(f);}
 var fgOwn=new Dictionary<string,string>(own){{InstallerEngine.FrameworkProxy,hash}};
 var fgDeps=new Dictionary<string,object>(dependencies){["ReShade"]=new{version="PR435",file="Dungeons/Binaries/Win64/d3d12.asi",sha256=hash,url="https://example.test/framework",bootstrapPreviousSHA256=oldHash}};
 var fgLock=JsonSerializer.Serialize(new{game=new{steamBuildID="123",exeSHA256=hash},dependencies=fgDeps});
 var fgManifest=JsonSerializer.Serialize(new{version="fg.test",files=fgOwn});
 byte[] FgPayload(){using var memory=new MemoryStream();using(var z=new ZipArchive(memory,ZipArchiveMode.Create,true))foreach(var f in fgOwn){using var w=z.CreateEntry(f.Key).Open();w.Write(file);}return memory.ToArray();}
 InstallerEngine FG(){var e=new InstallerEngine(fgLock,fgManifest,()=>new MemoryStream(FgPayload())){RequireClosed=()=>{},ConfigRootOverride=Path.Combine(temp,"FGConfig")};e.SetGame(temp);e.SelectDependency("DLSSRuntime",runtime);e.SelectDependency("ReShade",runtime);return e;}
 var proxy=InstallerEngine.Target(temp,InstallerEngine.FrameworkProxy);Directory.CreateDirectory(Path.GetDirectoryName(proxy)!);File.WriteAllBytes(proxy,oldBytes);
 var fg=FG();fg.Install();Check(InstallerEngine.DigestFile(proxy)==hash,"Pinned old graphics proxy upgraded");
 var recovery=InstallerEngine.Target(temp,"Dungeons/Binaries/Win64/MCD2GraphicsDependencyBackups/"+oldHash+".bak");Check(InstallerEngine.DigestFile(recovery)==oldHash,"Original graphics proxy recovery preserved");
 var frameworkZip=MakeZip("ReShade64.dll");var frameworkFile=Path.Combine(temp,"framework.zip");File.WriteAllBytes(frameworkFile,frameworkZip);
 var zipDeps=new Dictionary<string,object>(fgDeps){["ReShade"]=new{version="PR435",file="Dungeons/Binaries/Win64/d3d12.asi",sha256=hash,download="https://example.test/framework.zip",archiveSHA256=InstallerEngine.Digest(frameworkZip),archiveMembers=new Dictionary<string,string>{{"Dungeons/Binaries/Win64/d3d12.asi","ReShade64.dll"}},bootstrapPreviousSHA256=oldHash}};
 var zipLock=JsonSerializer.Serialize(new{game=new{steamBuildID="123",exeSHA256=hash},dependencies=zipDeps});
 var zipEngine=new InstallerEngine(zipLock,fgManifest,()=>new MemoryStream(FgPayload())){RequireClosed=()=>{}};zipEngine.SetGame(temp);zipEngine.SelectDependency("ReShade",frameworkFile);Check(zipEngine.Dependencies.Single(x=>x.Id=="ReShade").Url.EndsWith("framework.zip"),"Pinned single-file framework ZIP accepted");
 File.WriteAllText(frameworkFile,"wrong archive");Throws(()=>zipEngine.SelectDependency("ReShade",frameworkFile),"Wrong single-file framework archive refused");
 var fgMarker=InstallerEngine.Target(temp,InstallerEngine.Marker);var fgReceipt=File.ReadAllBytes(fgMarker);using(var parsed=JsonDocument.Parse(fgReceipt))File.WriteAllText(fgMarker,JsonSerializer.Serialize(new{version="fg.test",files=parsed.RootElement.GetProperty("files")}));
 Throws(()=>fg.Uninstall(),"Missing bootstrap ownership blocks removal");File.WriteAllBytes(fgMarker,fgReceipt);
 fg.Install(true);Check(InstallerEngine.DigestFile(recovery)==oldHash,"Repair preserves original bootstrap ownership");fg.Uninstall();Check(InstallerEngine.DigestFile(proxy)==oldHash,"Uninstall restores original graphics proxy");
 File.WriteAllText(proxy,"unknown");Throws(()=>FG().Install(),"Unknown graphics proxy refused");Check(File.ReadAllText(proxy)=="unknown"&&!File.Exists(InstallerEngine.Target(temp,InstallerEngine.Marker)),"Unknown proxy is unchanged");
 File.WriteAllBytes(proxy,oldBytes);fg=FG();fg.Install();File.WriteAllText(recovery,"edited");Throws(()=>fg.Uninstall(),"Modified bootstrap recovery prevents removal");Check(InstallerEngine.DigestFile(proxy)==hash,"Bootstrap retained on recovery failure");
 File.WriteAllBytes(recovery,oldBytes);fg.Uninstall();File.Delete(proxy);fg=FG();fg.Install();fg.Uninstall();Check(!File.Exists(proxy),"Fresh bootstrap removed when no original existed");
 // Overrides are explicit, scoped and cannot grant ownership of arbitrary paths.
 Put("Dungeons/Binaries/Win64/dxgi.dll");
 File.WriteAllBytes(runtime,Encoding.UTF8.GetBytes("new dependency"));var untested=Engine();
 Throws(()=>untested.SelectDependency("DLSSRuntime",runtime),"Strict default refuses newer DLSS");
 untested.SetDependencyOverride("DLSSRuntime",true);untested.SelectDependency("DLSSRuntime",runtime);
 Check(untested.Scan().Single(x=>x.Name=="NVIDIA DLSS runtime").State=="Untested override"&&untested.Ready,"Explicit newer DLSS is untested and installable");
 untested.Install();var actual=InstallerEngine.DigestFile(InstallerEngine.Target(temp,InstallerEngine.Runtime));
 Check(actual==InstallerEngine.DigestFile(runtime),"Override installs exactly selected DLSS bytes");
 var removal=Engine();removal.Uninstall();Check(!File.Exists(InstallerEngine.Target(temp,InstallerEngine.Runtime)),"Fresh installer can remove recorded private override safely");
 untested=Engine();untested.SetDependencyOverride("RenoDXUEExtended",true);untested.UseInstalledDependency("RenoDXUEExtended");
 Check(untested.Scan().Single(x=>x.Name=="RenoDX HDR support").State=="Untested override","Installed dependency explicitly acknowledged");
 Put("Dungeons/Binaries/Win64/renodx-ue-extended.addon64",Encoding.UTF8.GetBytes("changed after approval"));
 Check(!untested.Ready,"Changed dependency invalidates prior acknowledgement");
 untested.SetDependencyOverride("RenoDXUEExtended",false);Check(!untested.OverrideEnabled("RenoDXUEExtended"),"Turning override off restores strict pins");Put("Dungeons/Binaries/Win64/renodx-ue-extended.addon64");
 untested.SetDependencyOverride("DLSSRuntime",true);untested.SelectDependency("DLSSRuntime",runtime);untested.SetGame(game);Check(!untested.OverrideEnabled("DLSSRuntime"),"Changing game clears overrides and selections");
 blocked=Engine();blocked.RequireClosed=()=>throw new InvalidOperationException("Running game");Throws(()=>blocked.SetDependencyOverride("DLSSRuntime",true),"Running game blocks override changes");
 var archiveOverride=new InstallerEngine(unsafeLock,manifest,()=>new MemoryStream(Payload())){RequireClosed=()=>{}};archiveOverride.SetGame(temp);archiveOverride.SetDependencyOverride("BlueprintLoader",true);
 File.WriteAllBytes(zipFile,unsafeZip);Throws(()=>archiveOverride.SelectDependency("BlueprintLoader",zipFile),"Override retains archive traversal rejection");
 // Multi-file overrides still require one coherent set of every required member.
 var loaderFiles=new Dictionary<string,string>{{"Dungeons/Content/Paks/~mods/BlueprintLoader/a.pak",hash},{"Dungeons/Content/Paks/~mods/BlueprintLoader/a.ucas",hash},{"Dungeons/Content/Paks/~mods/BlueprintLoader/a.utoc",hash}};
 var loaderDeps=new Dictionary<string,object>(dependencies){["BlueprintLoader"]=new{version="2.2",files=loaderFiles,archiveSHA256=hash,url="https://www.nexusmods.com/minecraftdungeons2/mods/2",upgradeFromFileSets=new[]{loaderFiles}}};
 var loaderLock=JsonSerializer.Serialize(new{game=new{steamBuildID="123",exeSHA256=hash},dependencies=loaderDeps});
 InstallerEngine Loader(){var x=new InstallerEngine(loaderLock,manifest,()=>new MemoryStream(Payload())){RequireClosed=()=>{},ConfigRootOverride=Path.Combine(temp,"Config")};x.SetGame(temp);return x;}
 byte[] LoaderZip(bool missing=false,bool duplicate=false,bool linked=false){using var m=new MemoryStream();using(var z=new ZipArchive(m,ZipArchiveMode.Create,true)){foreach(var name in loaderFiles.Keys){if(missing&&name.EndsWith("utoc"))continue;var a=z.CreateEntry("Loader/"+Path.GetFileName(name));if(linked)a.ExternalAttributes=unchecked((int)0xa0000000);using(var t=a.Open())t.Write(Encoding.UTF8.GetBytes("new loader"));}if(duplicate){using var t=z.CreateEntry("Other/a.pak").Open();t.Write(file);}}return m.ToArray();}
 foreach(var name in loaderFiles.Keys)Put(name);
 var loader=Loader();File.WriteAllBytes(zipFile,LoaderZip());Throws(()=>loader.SelectDependency("BlueprintLoader",zipFile),"Newer multi-file dependency requires explicit override");loader.SetDependencyOverride("BlueprintLoader",true);loader.SelectDependency("BlueprintLoader",zipFile);Check(loader.Scan().Single(x=>x.Name=="Blueprint Loader").State=="Untested override","Complete newer loader archive acknowledged");
 File.WriteAllBytes(zipFile,LoaderZip(missing:true));Throws(()=>loader.SelectDependency("BlueprintLoader",zipFile),"Override refuses missing required archive member");
 File.WriteAllBytes(zipFile,LoaderZip(duplicate:true));Throws(()=>loader.SelectDependency("BlueprintLoader",zipFile),"Override refuses ambiguous archive member");
 File.WriteAllBytes(zipFile,LoaderZip(linked:true));Throws(()=>loader.SelectDependency("BlueprintLoader",zipFile),"Override refuses archive symbolic links");
 File.Delete(InstallerEngine.Target(temp,loaderFiles.Keys.Last()));var installedLoader=Loader();installedLoader.SetDependencyOverride("BlueprintLoader",true);Throws(()=>installedLoader.UseInstalledDependency("BlueprintLoader"),"Installed override refuses an incomplete dependency set");
 // Shared replacements keep a recovery copy and remain after removal.
 Put("Dungeons/Binaries/Win64/renodx-ue-extended.addon64");var shared=Engine();shared.SetDependencyOverride("RenoDXUEExtended",true);shared.SelectDependency("RenoDXUEExtended",runtime);
 File.WriteAllBytes(runtime,file);shared.SelectDependency("DLSSRuntime",runtime);shared.Install();Check(InstallerEngine.DigestFile(InstallerEngine.Target(temp,"Dungeons/Binaries/Win64/renodx-ue-extended.addon64"))==actual,"Shared override placed exactly");
 shared.Uninstall();Check(File.Exists(InstallerEngine.Target(temp,"Dungeons/Binaries/Win64/renodx-ue-extended.addon64")),"Shared override retained on uninstall");Put("Dungeons/Binaries/Win64/renodx-ue-extended.addon64");
 // Acknowledgements never bypass the game or owned payload checks.
 var integrity=Engine(()=>new MemoryStream(Encoding.UTF8.GetBytes("invalid payload")));integrity.SetDependencyOverride("DLSSRuntime",true);integrity.SelectDependency("DLSSRuntime",runtime);Throws(()=>integrity.Install(),"Override cannot bypass mod payload integrity");
 var gameGuard=Engine();gameGuard.SetDependencyOverride("DLSSRuntime",true);gameGuard.SelectDependency("DLSSRuntime",runtime);Put(InstallerEngine.Shipping,Encoding.UTF8.GetBytes("other build"));Check(!gameGuard.Ready,"Override cannot bypass supported game build");Put(InstallerEngine.Shipping);
 var tamper=Engine();tamper.SelectDependency("DLSSRuntime",runtime);tamper.Install();using(var parsed=JsonDocument.Parse(File.ReadAllBytes(InstallerEngine.Target(temp,InstallerEngine.Marker)))) {
 Put(InstallerEngine.Marker,Encoding.UTF8.GetBytes(JsonSerializer.Serialize(new{version="candidate.test",files=parsed.RootElement.GetProperty("files"),dependencyOverrides=new Dictionary<string,string>{{own.Keys.Single(),hash}}})));
 }
 Throws(()=>tamper.Uninstall(),"Override receipt cannot grant ownership of own payload");File.Delete(InstallerEngine.Target(temp,InstallerEngine.Marker));File.Delete(InstallerEngine.Target(temp,own.Keys.Single()));File.Delete(InstallerEngine.Target(temp,InstallerEngine.Runtime));
 File.WriteAllBytes(runtime,file);
 // Minimum APIs and complete recognized release sets are shared across dependencies.
 var compatibleFiles=loaderFiles.ToDictionary(x=>x.Key,x=>oldHash);
 var compatibleArchive=MakeZip(loaderFiles.Keys.Select(x=>"Loader/"+Path.GetFileName(x)).ToArray());
 var compatibleDeps=new Dictionary<string,object>(dependencies){["BlueprintLoader"]=new{version="2.2",minimumVersion="2.0",files=compatibleFiles,archiveSHA256="unused",url="https://www.nexusmods.com/minecraftdungeons2/mods/2",compatibleReleases=new[]{new{version="2.3",files=loaderFiles,archiveSHA256=InstallerEngine.Digest(compatibleArchive)}}}};
 string CompatLock()=>JsonSerializer.Serialize(new{game=new{steamBuildID="123",exeSHA256=hash},dependencies=compatibleDeps});
 InstallerEngine Compatible(){var x=new InstallerEngine(CompatLock(),manifest,()=>new MemoryStream(Payload())){RequireClosed=()=>{},ConfigRootOverride=Path.Combine(temp,"Config")};x.SetGame(temp);return x;}
 foreach(var path in loaderFiles.Keys)Put(path);
 var compatible=Compatible();Check(compatible.Dependencies.Single(x=>x.Id=="BlueprintLoader").Requirement=="2.0+","Loader API minimum displayed with plus policy");
 Check(compatible.Scan().Single(x=>x.Name=="Blueprint Loader") is {State:"OK",Version:"2.3"},"Recognized installed newer release satisfies requirement without override or upgrade");
 File.WriteAllBytes(zipFile,compatibleArchive);compatible.SelectDependency("BlueprintLoader",zipFile);
 Check(!compatible.OverrideEnabled("BlueprintLoader")&&compatible.Scan().Single(x=>x.Name=="Blueprint Loader").Version=="2.3","Recognized newer archive accepted without override");
 foreach(var path in loaderFiles.Keys)File.Delete(InstallerEngine.Target(temp,path));
 var freshCompatible=Compatible();freshCompatible.SelectDependency("BlueprintLoader",zipFile);freshCompatible.SelectDependency("DLSSRuntime",runtime);
 freshCompatible.Install();Check(loaderFiles.All(x=>InstallerEngine.DigestFile(InstallerEngine.Target(temp,x.Key))==x.Value),"Fresh install uses whole recognized release rather than default pins");freshCompatible.Uninstall();
 var mixed=Compatible();Put(loaderFiles.Keys.Last(),oldBytes);Check(mixed.Scan().Single(x=>x.Name=="Blueprint Loader").State=="Unsupported version","Mixed individually recognized files are not a coherent release");
 foreach(var path in loaderFiles.Keys)Put(path,Encoding.UTF8.GetBytes("unknown future loader"));
 var future=Compatible();Check(future.Scan().Single(x=>x.Name=="Blueprint Loader").State=="Unsupported version","Unknown future installed loader requires explicit override");future.SetDependencyOverride("BlueprintLoader",true);future.UseInstalledDependency("BlueprintLoader");Check(future.Scan().Single(x=>x.Name=="Blueprint Loader").State=="Untested override","Future installed loader can be tried explicitly");
 var belowMinimum=new Dictionary<string,object>(compatibleDeps){["BlueprintLoader"]=new{version="2.2",minimumVersion="2.0",files=loaderFiles,url="https://example.test",compatibleReleases=new[]{new{version="1.1",files=loaderFiles}}}};
 Throws(()=>new InstallerEngine(JsonSerializer.Serialize(new{game=new{steamBuildID="123",exeSHA256=hash},dependencies=belowMinimum}),manifest,()=>null),"Recognized set below minimum API refused");
 var incompleteSet=new Dictionary<string,object>(compatibleDeps){["BlueprintLoader"]=new{version="2.2",files=loaderFiles,url="https://example.test",compatibleReleases=new[]{new{version="2.3",files=new Dictionary<string,string>{{loaderFiles.Keys.First(),hash}}}}}};
 Throws(()=>new InstallerEngine(JsonSerializer.Serialize(new{game=new{steamBuildID="123",exeSHA256=hash},dependencies=incompleteSet}),manifest,()=>null),"Incomplete recognized release declaration refused");
 foreach(var id in new[]{"ReShade","RenoDXUEExtended","BlueprintLoader","DLSSRuntime","Streamline"}) {
  var depPath=id=="DLSSRuntime"?InstallerEngine.Runtime:"Dungeons/Binaries/Win64/"+id+"-policy-test.dll";
  var policyDeps=new Dictionary<string,object>(dependencies){[id]=new{version="2.2",minimumVersion="2.0",file=depPath,sha256=oldHash,url="https://example.test",compatibleReleases=new[]{new{version="2.3",files=new Dictionary<string,string>{{depPath,hash}}}}}};
  var policyJson=JsonSerializer.Serialize(new{game=new{steamBuildID="123",exeSHA256=hash},dependencies=policyDeps});
  var policy=new InstallerEngine(policyJson,manifest,()=>new MemoryStream(Payload())){RequireClosed=()=>{},ConfigRootOverride=Path.Combine(temp,"Config")};policy.SetGame(temp);Put(depPath);
  Check(policy.Scan().Any(x=>x.Version=="2.3"&&x.State=="OK"),id+" shares recognized newer release policy");
  policy.SetDependencyOverride(id,true);Put(depPath,oldBytes);policy.UseInstalledDependency(id);Check(policy.Scan().Any(x=>x.State=="Untested override"),id+" has explicit installed-version override");
  policy.SetDependencyOverride(id,false);Put(depPath);File.WriteAllBytes(runtime,file);policy.SelectDependency(id,runtime);
  Check(!policy.OverrideEnabled(id),id+" recognized single file accepted without override");
  if(id=="DLSSRuntime"){policy.Install();policy.Uninstall();Check(!File.Exists(InstallerEngine.Target(temp,depPath)),"Recognized alternate private runtime can be removed by receipt");}
 }
 // Validate the shipped policy, plus optional official archive fixtures locally.
 using(var actualPolicy=JsonDocument.Parse(File.ReadAllText("dependencies.lock.json"))) {
  var policyJson=JsonSerializer.Serialize(new{game=new{steamBuildID="123",exeSHA256=hash},dependencies=actualPolicy.RootElement.GetProperty("dependencies")});
  InstallerEngine ActualPolicy(){var x=new InstallerEngine(policyJson,manifest,()=>new MemoryStream(Payload())){RequireClosed=()=>{}};x.SetGame(temp);return x;}
  var actualEngine=ActualPolicy();Check(actualEngine.Dependencies.Count==5&&actualEngine.Dependencies.Single(x=>x.Id=="BlueprintLoader").MinimumVersion=="2.0","Shipped policy has five dependencies and Loader 2.0 minimum");
  foreach(var dep in actualEngine.Dependencies){actualEngine.SetDependencyOverride(dep.Id,true);Check(actualEngine.OverrideEnabled(dep.Id),dep.Id+" shipped override available");actualEngine.SetDependencyOverride(dep.Id,false);}
  foreach(var archivePath in args) {
   actualEngine.SelectDependency("BlueprintLoader",archivePath);var status=actualEngine.Scan().Single(x=>x.Name=="Blueprint Loader");Check(status.State=="Ready to install"&&!actualEngine.OverrideEnabled("BlueprintLoader"),"Official Loader "+status.Version+" archive matches shipped complete set");
   using(var archiveInput=ZipFile.OpenRead(archivePath))foreach(var entry in archiveInput.Entries.Where(x=>x.Name.EndsWith(".pak")||x.Name.EndsWith(".ucas")||x.Name.EndsWith(".utoc"))){using var input=entry.Open();using var bytes=new MemoryStream();input.CopyTo(bytes);Put("Dungeons/Content/Paks/~mods/BlueprintLoader/"+entry.Name,bytes.ToArray());}
   Check(ActualPolicy().Scan().Single(x=>x.Name=="Blueprint Loader").State=="OK","Installed official Loader "+status.Version+" recognized without upgrade");
  }
 }
}finally{Directory.Delete(temp,true);}
Console.WriteLine($"{cases} checks passed");
