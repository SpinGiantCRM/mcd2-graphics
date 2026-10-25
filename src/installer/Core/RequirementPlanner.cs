using System.Text.Json;
namespace MCD2.Installer;
public sealed record AcquisitionRoute(string Method,string Url);
public sealed record InstallContext(GameEdition Edition,GameEnvironment Environment,string AdapterEvidence="unknown",IReadOnlySet<string>? Features=null);
public sealed record DependencyRequirement(Dependency Dependency,bool Required,string Reason,string? InstalledRelease,AcquisitionRoute Acquisition);
public sealed record RequirementPlan(IReadOnlyList<DependencyRequirement> Requirements);
public static class RequirementPlanner {
 public static RequirementPlan Resolve(InstallContract contract,InstallContext context,IReadOnlyList<Dependency> dependencies,IReadOnlyList<Check> checks) {
  if(!contract.Data.GetProperty("editionProfiles").TryGetProperty(context.Edition.ToString(),out _))throw new InvalidDataException("This package does not support that edition.");
  var result=new List<DependencyRequirement>();
  foreach(var d in contract.Data.GetProperty("dependencies").EnumerateArray()) {
   if(!d.GetProperty("editions").EnumerateArray().Any(x=>x.GetString()==context.Edition.ToString()))continue;
   var dep=dependencies.Single(x=>x.Id==d.GetProperty("id").GetString());var status=checks.Single(x=>x.Name==dep.Name);var a=d.GetProperty("acquisition");
   // Complete closure is deliberate: no vendor heuristics or unsupported optional admission.
   result.Add(new(dep,true,d.GetProperty("reason").GetString()!,status.State=="OK"?status.Version:null,new(a.GetProperty("method").GetString()!,a.GetProperty("url").GetString()!)));
  }
  return new(result.AsReadOnly());
 }
}
