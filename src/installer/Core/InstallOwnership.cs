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
   if(!File.Exists(marker))return new(InstallOwner.Unmanaged,"Ready for this manager after checking requirements.");
   try {
    var r=ReadReceipt(marker);var owner=InstallOwnership.Check(r);
    return new(owner,owner switch {
     InstallOwner.Vortex=>"Vortex owns this installation. In Vortex, disable MCD2 Graphics and Deploy, or choose Purge. Keep shared requirements and saved preferences. Then return here and choose Check again.",
     InstallOwner.Standalone=>"To switch to Vortex, choose Prepare for Vortex below. This removes unchanged owned mod files, restores owned startup settings, and keeps shared requirements, preferences and saves. Then install and enable the matching Vortex package for this game.",
     _=>"Unknown installation receipt. Keep these files and use the original manager to remove the installation. Do not delete DLLs manually."
    });
   }catch(Exception e)when(e is System.Text.Json.JsonException or InvalidDataException){return new(InstallOwner.Conflict,"This installation receipt needs attention. Keep it and use the original installer or restore a trusted backup.");}
  }
 }
}
