using NeoRune.Assets;
using NeoRune.Packaging;
using UAssetAPI.ExportTypes;
using UAssetAPI.PropertyTypes.Objects;
using UAssetAPI.UnrealTypes;
using System.Text.Json;
using System.Diagnostics;
if(args.Length!=4)throw new ArgumentException("assets-directory pack-tools output-directory manifest.json");
var version=JsonDocument.Parse(File.ReadAllText(args[3])).RootElement.GetProperty("version").GetString()!;
var p=new PackageBuilder("/Game/Mods/MCD2Graphics/ModInfo");
const string type="/Game/Mods/BlueprintLoader/BP_ModInfo.BP_ModInfo_C";
var e=new NormalExport { ObjectName=p.Name("ModInfo"), ClassIndex=p.ImportClass(type), TemplateIndex=p.ImportDefaultObject(type), ObjectFlags=EObjectFlags.RF_Public|EObjectFlags.RF_Standalone, bIsAsset=true, Data=new(), Extras=Array.Empty<byte>() };
foreach(var pair in new Dictionary<string,string> {
 ["ModName"]="MCD2 Graphics", ["Version"]=version, ["Author"]="SpinGiantCRM", ["AuthorUrl"]="https://github.com/SpinGiantCRM/mcd2-graphics",
 ["Description"]="DLSS Super Resolution, DLAA, native HDR controls and supported NVIDIA Reflex. Graphics settings are in Settings → Video. Advanced HDR processing remains in the RenoDX ReShade panel. Experimental features remain hidden until supported.\n\nInstallation, requirements and support: https://github.com/SpinGiantCRM/mcd2-graphics\nUse offline; online compatibility is not established."
})e.Data.Add(new StrPropertyData(p.Name(pair.Key)){Value=new FString(pair.Value),PropertyTypeName=p.TypeName(("StrProperty",0))});
p.AddExport(e);p.Write(args[0],"ModInfo");
var temporary=Path.Combine(args[2],"MetadataTemp");
new ModPackager(args[1]).Pack("MCD2Graphics",args[0],args[2],temporary);
// The loader discovers metadata through the pak header. The full asset data stays in IoStore.
var header=Path.Combine(temporary,"Header");
File.Copy(Path.Combine(args[0],"ModInfo.uasset"),Path.Combine(header,"Dungeons","Content","Mods","MCD2Graphics","ModInfo.uasset"),true);
var pack=new ProcessStartInfo(Path.Combine(args[1],"repak.exe")){UseShellExecute=false};
foreach(var a in new[]{"pack","-q","--version","V11",header,Path.Combine(args[2],"MCD2Graphics_P.pak")})pack.ArgumentList.Add(a);
using(var process=Process.Start(pack)??throw new InvalidOperationException("Packer unavailable")){process.WaitForExit();if(process.ExitCode!=0)throw new InvalidOperationException("Metadata header packing failed");}
Console.WriteLine("ModInfo built; metadata uses Blueprint Loader 2.0's BP_ModInfo class.");
