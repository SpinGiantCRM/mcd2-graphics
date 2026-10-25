using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
namespace MCD2.Installer;

public sealed class InstallContract {
 public JsonElement Data {get;}
 public string Identity {get;}
 InstallContract(JsonElement data) {Data=data;Identity=CanonicalHash(data);}
 public static InstallContract Load(Stream input) {
  using var memory=new MemoryStream();var buffer=new byte[8192];int n;
  while((n=input.Read(buffer))>0){if(memory.Length+n>4*1024*1024)throw new InvalidDataException("Install contract too large.");memory.Write(buffer,0,n);}
  try {
   using var doc=JsonDocument.Parse(memory.ToArray());var d=doc.RootElement;
   RejectDuplicates(d);
   if(d.GetProperty("schemaVersion").GetInt32()!=1||d.GetProperty("kind").GetString()!="mcd2-graphics")throw new InvalidDataException("Unsupported install contract.");
   var version=d.GetProperty("packageVersion").GetString();if(version==null||!Regex.IsMatch(version,"^[a-zA-Z0-9.+_-]{1,100}$"))throw new InvalidDataException("Invalid package version.");
   ValidateHashes(d.GetProperty("payload"),true);
   foreach(var edition in d.GetProperty("editionProfiles").EnumerateObject()) {
    var bin=edition.Name switch {"Steam"=>"Dungeons/Binaries/Win64","WinGDK"=>"Dungeons/Binaries/WinGDK",_=>throw new InvalidDataException("Unknown edition.")};
    if(edition.Value.GetProperty("binaryDirectory").GetString()!=bin||edition.Value.GetProperty("shipping").GetString()!=bin+"/Dungeons-"+(edition.Name=="Steam"?"Win64":"WinGDK")+"-Shipping.exe")throw new InvalidDataException("Invalid edition layout.");
    Pin(edition.Value.GetProperty("exeSHA256").GetString());
   }
   var names=new HashSet<string>();
   foreach(var dep in d.GetProperty("dependencies").EnumerateArray()) {
    if(!names.Add(dep.GetProperty("id").GetString()!))throw new InvalidDataException("Duplicate requirement.");
    ValidateHashes(dep.GetProperty("files"),false);
    foreach(var release in dep.GetProperty("releases").EnumerateArray()) {
     ValidateHashes(release.GetProperty("files"),false);
     if(!dep.GetProperty("files").EnumerateObject().Select(x=>x.Name).Order().SequenceEqual(release.GetProperty("files").EnumerateObject().Select(x=>x.Name).Order()))throw new InvalidDataException("Mixed release members.");
    }
    var a=dep.GetProperty("acquisition");
    if(a.GetProperty("method").GetString() is not ("browser" or "direct")||!Uri.TryCreate(a.GetProperty("url").GetString(),UriKind.Absolute,out var url)||url.Scheme!="https")throw new InvalidDataException("Invalid acquisition route.");
   }
   return new(d.Clone());
  }catch(Exception e)when(e is JsonException or KeyNotFoundException or InvalidOperationException or FormatException){throw new InvalidDataException("Invalid install contract.",e);}
 }
 public void ValidateInputs(string manifestJson,string dependencyJson) {
  using var m=JsonDocument.Parse(manifestJson);using var l=JsonDocument.Parse(dependencyJson);
  if(Data.GetProperty("manifestIdentity").GetString()!=CanonicalHash(m.RootElement)||Data.GetProperty("dependencyIdentity").GetString()!=CanonicalHash(l.RootElement)||Data.GetProperty("packageVersion").GetString()!=m.RootElement.GetProperty("version").GetString()||CanonicalHash(Data.GetProperty("payload"))!=CanonicalHash(m.RootElement.GetProperty("files")))throw new InvalidDataException("Install contract differs from the selected package inputs.");
 }
 static void RejectDuplicates(JsonElement value) {
  if(value.ValueKind==JsonValueKind.Object){var names=new HashSet<string>(StringComparer.OrdinalIgnoreCase);foreach(var p in value.EnumerateObject()){if(!names.Add(p.Name))throw new InvalidDataException("Duplicate contract property.");RejectDuplicates(p.Value);}}
  else if(value.ValueKind==JsonValueKind.Array)foreach(var item in value.EnumerateArray())RejectDuplicates(item);
 }
 static void Pin(string? hash){if(hash==null||!Regex.IsMatch(hash,"^[a-f0-9]{64}$"))throw new InvalidDataException("Invalid SHA-256 pin.");}
 static void ValidateHashes(JsonElement files,bool owned){if(!files.EnumerateObject().Any())throw new InvalidDataException("Empty file set.");foreach(var p in files.EnumerateObject()){InstallerEngine.ValidateRelative(p.Name);if(!p.Name.StartsWith("Dungeons/Binaries/Win64/",StringComparison.Ordinal)&&!p.Name.StartsWith("Dungeons/Content/Paks/~mods/",StringComparison.Ordinal))throw new InvalidDataException("Unsupported destination.");Pin(p.Value.GetString());var name=Path.GetFileName(p.Name).ToLowerInvariant();if(owned&&(name.StartsWith("sl.")||name.StartsWith("nvngx_")||name.StartsWith("amd_fidelityfx_")||name is "nvlowlatencyvk.dll" or "reshade64.dll" or "d3d12.asi" or "dlss-enabler-headless.dll" or "xgameruntime.thunks.dll"))throw new InvalidDataException("External runtime in owned payload.");}}
 public static string CanonicalHash(JsonElement value){using var memory=new MemoryStream();using(var w=new Utf8JsonWriter(memory,new(){Encoder=System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping})){Write(w,value);}return Convert.ToHexStringLower(SHA256.HashData(memory.ToArray()));}
 static void Write(Utf8JsonWriter w,JsonElement v){if(v.ValueKind==JsonValueKind.Object){w.WriteStartObject();foreach(var p in v.EnumerateObject().OrderBy(x=>x.Name,StringComparer.Ordinal)){w.WritePropertyName(p.Name);Write(w,p.Value);}w.WriteEndObject();}else if(v.ValueKind==JsonValueKind.Array){w.WriteStartArray();foreach(var a in v.EnumerateArray())Write(w,a);w.WriteEndArray();}else v.WriteTo(w);}
}
