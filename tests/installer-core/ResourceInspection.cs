using System.Reflection.Metadata;
using System.Reflection.PortableExecutable;
using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text.Json;
// Metadata-only inspection: never load or execute the packaged GUI assembly.
static class ResourceInspection {
 public static void Run(string path){
  using var stream=File.OpenRead(path);using var pe=new PEReader(stream);var metadata=pe.GetMetadataReader();
  var data=pe.GetSectionData(pe.PEHeaders.CorHeader!.ResourcesDirectory.RelativeVirtualAddress).GetContent();var result=new Dictionary<string,string>();
  foreach(var handle in metadata.ManifestResources){var r=metadata.GetManifestResource(handle);var name=metadata.GetString(r.Name);if(name is not ("manifest.json" or "dependencies.lock.json" or "mcd2-install.json"))continue;if(!r.Implementation.IsNil)throw new InvalidDataException("Expected embedded input.");int offset=checked((int)r.Offset);var size=BinaryPrimitives.ReadInt32LittleEndian(data.AsSpan().Slice(offset,4));result[name]=Convert.ToHexStringLower(SHA256.HashData(data.AsSpan().Slice(offset+4,size)));}
  Console.WriteLine(JsonSerializer.Serialize(result));
 }
}
