using MCD2.Installer;
using System.Net;
using System.Text.Json;
static class AcquisitionTests {
 public static void Run(Action<bool,string> check,Action<Action,string> throws,string temp) {
  var dir=Path.Combine(temp,"acquisition");Directory.CreateDirectory(dir);byte[] b="verified dependency"u8.ToArray();string h=InstallerEngine.Digest(b);
  var dep=new Dependency("runtime","Runtime","1","https://publisher.example/file.dll",new(){{InstallerEngine.Runtime,h}});
  var requirement=new DependencyRequirement(dep,true,"Required",null,new("direct",dep.Url));
  var transport=new HttpClient(new BytesHandler(b));var a=new DependencyAcquisition(dir,transport);
  var staged=a.StageAsync(requirement,CancellationToken.None).GetAwaiter().GetResult();
  check(InstallerEngine.DigestFile(staged.Path)==h,"Acquisition stages only expected bytes in cache");
  File.WriteAllText(staged.Path,"corrupt cache");
  var restored=a.StageAsync(requirement,CancellationToken.None).GetAwaiter().GetResult();
  check(InstallerEngine.DigestFile(restored.Path)==h,"Corrupt cache is not trusted");
  var wrong=new DependencyAcquisition(Path.Combine(dir,"bad"),new HttpClient(new BytesHandler("wrong"u8.ToArray())));
  throws(()=>wrong.StageAsync(requirement,CancellationToken.None).GetAwaiter().GetResult(),"Wrong downloaded hash never becomes a staged dependency");
  check(!Directory.Exists(Path.Combine(dir,"bad"))||!Directory.EnumerateFiles(Path.Combine(dir,"bad")).Any(),"Failed downloads leave no cache entry or partial file");
  throws(()=>a.StageAsync(requirement with {Acquisition=new("browser",dep.Url)},CancellationToken.None).GetAwaiter().GetResult(),"Browser/login requirements cannot be fetched silently");
 }
 sealed class BytesHandler(byte[] bytes):HttpMessageHandler {
  protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,CancellationToken cancellationToken)=>Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK){Content=new ByteArrayContent(bytes)});
 }
}
