using MCD2.Installer;
using System.Text.Json;

static class InstallPolicyTests {
 public static void Run(Action<bool,string> check,Action<Action,string> throws,string temp) {
  var root=Path.Combine(temp,"snapshot");Directory.CreateDirectory(Path.Combine(root,"Dungeons/Binaries/Win64"));
  byte[] b="snapshot payload"u8.ToArray();string h=InstallerEngine.Digest(b);
  File.WriteAllBytes(Path.Combine(root,InstallerEngine.Shipping),b);
  var files=new Dictionary<string,string>{{"Dungeons/Binaries/Win64/mcd2-graphics.addon64",h}};
  var dep=new{version="1",file=InstallerEngine.Runtime,sha256=h,url="https://example.com/dlss"};
  var l=JsonSerializer.Serialize(new{game=new{steamBuildID="1",exeSHA256=h},dependencies=new{DLSSRuntime=dep}});
  var m=JsonSerializer.Serialize(new{version="test",files});
  var payload=Path.Combine(temp,"snapshot-payload");Directory.CreateDirectory(Path.Combine(payload,"Dungeons/Binaries/Win64"));File.WriteAllBytes(Path.Combine(payload,files.Keys.Single()),b);
  var e=new InstallerEngine(l,m,payload){RequireClosed=()=>{}};e.SetGame(root);
  var first=e.Snapshot;File.WriteAllText(Path.Combine(root,InstallerEngine.Shipping),"changed after scan");
  check(ReferenceEquals(first,e.Snapshot),"UI redraw reuses immutable scan, without hashing again");
  throws(()=>e.Install(),"Mutation revalidates changed executable even when UI snapshot was ready");
  check(!File.Exists(Path.Combine(root,files.Keys.Single())),"Failed fresh preflight performs no payload writes");
  File.WriteAllBytes(Path.Combine(root,InstallerEngine.Shipping),b);
  var marker=Path.Combine(root,InstallerEngine.Marker);Directory.CreateDirectory(Path.GetDirectoryName(marker)!);
  File.WriteAllText(marker,JsonSerializer.Serialize(new{schemaVersion=1,owner="vortex",version="test",files}));
  throws(()=>e.Install(true),"Standalone cannot adopt Vortex deployment");
  throws(()=>e.Uninstall(),"Standalone cannot remove Vortex deployment");
  File.Delete(marker);
  check(e.RefreshScan().Checks[0].State=="OK","Explicit rescan sees restored executable");
 }
}
