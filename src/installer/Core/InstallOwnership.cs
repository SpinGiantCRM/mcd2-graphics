namespace MCD2.Installer;

public enum InstallOwner { Unmanaged, Standalone, Vortex, Conflict }
public static class InstallOwnership {
 public static InstallOwner Check(Receipt? receipt) => receipt==null?InstallOwner.Unmanaged:
  receipt.SchemaVersion==0&&receipt.Owner==null?InstallOwner.Standalone:
  receipt.SchemaVersion==1?receipt.Owner switch {"standalone"=>InstallOwner.Standalone,"vortex"=>InstallOwner.Vortex,_=>InstallOwner.Conflict}:InstallOwner.Conflict;
 public static void RequireStandalone(Receipt receipt) {
  var owner=Check(receipt);
  if(owner==InstallOwner.Vortex)throw new InvalidDataException("Vortex owns this installation. Purge it in Vortex before using the standalone installer. Your preferences and shared requirements can stay.");
  if(owner!=InstallOwner.Standalone)throw new InvalidDataException("Unknown installation ownership. Files were retained; use the original manager to remove this installation.");
 }
}

public sealed record InstallTransition(InstallOwner Owner,string NextAction);
public sealed partial class InstallerEngine {
 public InstallTransition Transition {
  get {
   if(GameRoot==null)return new(InstallOwner.Unmanaged,"Choose the game first.");
   var marker=Target(GameRoot,Profile!.MapDestination(Marker));
   if(!File.Exists(marker)){
    try{if(VortexManagedFiles().Contains(Profile.MapDestination(Runtime)))return new(InstallOwner.Vortex,"Vortex still owns the private DLSSRuntime (nvngx_dlss.dll) requirement. Disable that requirement in Vortex and Deploy, or Purge, then Check again. Other shared requirements and saved preferences can stay.");}
    catch(Exception error)when(error is InvalidDataException or IOException or UnauthorizedAccessException){return new(InstallOwner.Conflict,"Resolve this game's deployment metadata in Vortex before changing files.");}
    return new(InstallOwner.Unmanaged,"Ready for this manager after checking requirements.");
   }
   try {
    var r=ReadReceipt(marker);var owner=InstallOwnership.Check(r);
    return new(owner,owner switch {
     InstallOwner.Vortex=>"Vortex owns this installation. In Vortex, disable MCD2 Graphics and its private DLSSRuntime (nvngx_dlss.dll) requirement, then Deploy, or choose Purge. Other shared requirements and saved preferences can stay. Then return here and choose Check again.",
     InstallOwner.Standalone=>"To switch to Vortex, choose Prepare for Vortex below. This removes unchanged owned mod files, restores owned startup settings, and keeps shared requirements, preferences and saves. Then install and enable the matching Vortex package for this game.",
     _=>"Unknown installation receipt. Keep these files and use the original manager to remove the installation. Do not delete DLLs manually."
    });
   }catch(Exception e)when(e is System.Text.Json.JsonException or InvalidDataException or IOException or UnauthorizedAccessException){return new(InstallOwner.Conflict,"This installation receipt needs attention. Keep it and use the original installer or restore a trusted backup.");}
  }
 }
}

public sealed partial class InstallerEngine {
 HashSet<string> VortexManagedFiles(){
  var result=new HashSet<string>(OperatingSystem.IsWindows()?StringComparer.OrdinalIgnoreCase:StringComparer.Ordinal);
  var path=Target(GameRoot!,"vortex.deployment.mcd2-graphics-root.json");
  if(!File.Exists(path))return result;
  if(new FileInfo(path).Length>4*1024*1024)throw new InvalidDataException("Vortex deployment metadata needs attention. Resolve it in Vortex before changing files.");
  try{
   using var document=System.Text.Json.JsonDocument.Parse(File.ReadAllBytes(path));var data=document.RootElement;
   if(data.GetProperty("version").GetInt32()!=1||data.GetProperty("gameId").GetString()!="minecraftdungeons2")throw new InvalidDataException("Unknown Vortex deployment metadata; use Vortex to remove this installation.");
   foreach(var file in data.GetProperty("files").EnumerateArray()){
    var relative=(file.GetProperty("relPath").GetString()??throw new InvalidDataException("Invalid Vortex deployment path.")).Replace('\\','/');var prefix=file.TryGetProperty("target",out var target)?target.GetString():null;
    if(!string.IsNullOrEmpty(prefix))relative=prefix.Replace('\\','/')+"/"+relative;
    ValidateRelative(relative);result.Add(relative);
   }
  }catch(Exception error)when(error is System.Text.Json.JsonException or KeyNotFoundException or InvalidOperationException){throw new InvalidDataException("Vortex deployment metadata needs attention. Resolve it in Vortex before changing files.",error);}
  return result;
 }
 void ProtectVortexOwnership(IEnumerable<string> canonicalPaths){
  var managed=VortexManagedFiles();
  if(canonicalPaths.Any(path=>managed.Contains(Profile!.MapDestination(path))))throw new InvalidDataException("Vortex still owns files needed by this action. Disable MCD2 Graphics and its private DLSSRuntime requirement, then Deploy or Purge in Vortex. Shared requirements can stay if this installer will only reuse them.");
 }
}
