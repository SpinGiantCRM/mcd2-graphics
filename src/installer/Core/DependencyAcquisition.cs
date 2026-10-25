using System.Net.Http;
namespace MCD2.Installer;
public sealed record VerifiedDependency(string Path,string SHA256);
public sealed record AcquisitionProgress(string Requirement,long Received,long? Total);

// Downloads never touch game files. SelectDependency still validates full releases.
public sealed class DependencyAcquisition {
 const long Limit=512L*1024*1024;
 readonly string cache;readonly HttpClient client;
 public DependencyAcquisition(string cacheDirectory,HttpClient? http=null){cache=Path.GetFullPath(cacheDirectory);client=http??new HttpClient(new HttpClientHandler{AllowAutoRedirect=false}){Timeout=TimeSpan.FromMinutes(10)};}
 public static IReadOnlyList<string> AcceptedHashes(Dependency d)=>d.Releases.SelectMany(r=>r.ArchiveHash!=null?(IEnumerable<string>)new[]{r.ArchiveHash}:r.Files.Count==1?r.Files.Values:Array.Empty<string>()).Distinct().ToArray();
 public string? FindCached(Dependency dependency){if(!Directory.Exists(cache))return null;foreach(var h in AcceptedHashes(dependency)){var p=InstallerEngine.Target(cache,h);if(File.Exists(p)&&new FileInfo(p).Length<=Limit&&InstallerEngine.DigestFile(p)==h)return p;}return null;}
 public async Task<VerifiedDependency> StageAsync(DependencyRequirement requirement,CancellationToken cancellation,IProgress<AcquisitionProgress>? progress=null){
  if(requirement.Acquisition.Method!="direct")throw new InvalidDataException("Open the publisher's download page and import the file. Login or licence prompts need your attention.");
  if(!Uri.TryCreate(requirement.Acquisition.Url,UriKind.Absolute,out var url)||url.Scheme!="https"||!string.IsNullOrEmpty(url.UserInfo))throw new InvalidDataException("Only the approved HTTPS route can be used.");
  cancellation.ThrowIfCancellationRequested();
  if(FindCached(requirement.Dependency) is {} existing)return new(existing,InstallerEngine.DigestFile(existing));
  Directory.CreateDirectory(cache);var temp=InstallerEngine.Target(cache,"download-"+Guid.NewGuid().ToString("N")+".part");
  try{
   using var response=await DownloadResponse(url,cancellation);
   response.EnsureSuccessStatusCode();var total=response.Content.Headers.ContentLength;if(total>Limit)throw new InvalidDataException("Download exceeds the size limit.");
   await using(var input=await response.Content.ReadAsStreamAsync(cancellation))await using(var output=new FileStream(temp,FileMode.CreateNew,FileAccess.Write,FileShare.None,81920,true)){
    byte[] buffer=new byte[81920];long received=0;int count;
    while((count=await input.ReadAsync(buffer,cancellation))>0){received+=count;if(received>Limit)throw new InvalidDataException("Download exceeds the size limit.");await output.WriteAsync(buffer.AsMemory(0,count),cancellation);progress?.Report(new(requirement.Dependency.Name,received,total));}
    await output.FlushAsync(cancellation);
   }
   var hash=InstallerEngine.DigestFile(temp);
   if(!AcceptedHashes(requirement.Dependency).Contains(hash))throw new InvalidDataException("The publisher file does not match a supported release. Nothing was installed.");
   var target=InstallerEngine.Target(cache,hash);File.Move(temp,target,true);return new(target,hash);
  }finally{File.Delete(temp);}
 }
 async Task<HttpResponseMessage> DownloadResponse(Uri url,CancellationToken cancellation){
  for(int redirects=0;redirects<5;redirects++){
   var response=await client.GetAsync(url,HttpCompletionOption.ResponseHeadersRead,cancellation);
   if((int)response.StatusCode<300||(int)response.StatusCode>=400)return response;
   var location=response.Headers.Location;response.Dispose();
   if(location==null)throw new InvalidDataException("Publisher redirect is missing. Open its download page instead.");
   var next=location.IsAbsoluteUri?location:new Uri(url,location);
   bool allowed=next.Host==url.Host||url.Host=="github.com"&&(next.Host=="release-assets.githubusercontent.com"||next.Host=="objects.githubusercontent.com");
   if(next.Scheme!="https"||!string.IsNullOrEmpty(next.UserInfo)||!allowed)throw new InvalidDataException("Publisher redirected outside its approved download hosts. Use its official page and import the file.");
   url=next;
  }
  throw new InvalidDataException("Publisher redirected too many times. Use its official page and import the file.");
 }
 public IReadOnlyList<string> FindDownloads(string directory,IReadOnlyList<Dependency> dependencies){
  if(!Directory.Exists(directory))return [];
  var pins=dependencies.SelectMany(AcceptedHashes).ToHashSet();var result=new List<string>();long total=0;
  foreach(var f in new DirectoryInfo(directory).EnumerateFiles().Where(f=>f.Extension.ToLowerInvariant() is ".zip" or ".dll" or ".asi" or ".addon64").OrderByDescending(f=>f.LastWriteTimeUtc).Take(40)){
   if(f.Length<=0||f.Length>Limit||(f.Attributes&FileAttributes.ReparsePoint)!=0)continue;
   if(total+f.Length>2L*1024*1024*1024)break;total+=f.Length;
   if(pins.Contains(InstallerEngine.DigestFile(f.FullName)))result.Add(f.FullName);
  }
  return result.AsReadOnly();
 }
}
