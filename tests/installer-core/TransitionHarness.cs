using MCD2.Installer;
using System.Text.Json;
// Test-only bridge for cross-adapter filesystem fixtures. Never shipped in GUI.
static class TransitionHarness {
 public static void Run(string[] args){
  var root=Path.GetFullPath(args[2]);if(!File.Exists(Path.Combine(root,".synthetic-install-fixture")))throw new InvalidDataException("Synthetic fixture marker required.");
  var manifest=File.ReadAllText(args[4]);var pins=File.ReadAllText(args[5]);
  var e=new InstallerEngine(pins,manifest,args[3]){RequireClosed=()=>{}};e.SetGame(root);e.UseContract(File.ReadAllText(args[6]),manifest,pins);
  switch(args[1]){
   case "install":e.SelectDependency("DLSSRuntime",Path.Combine(args[3],InstallerEngine.Runtime));e.Install();break;
   case "remove":e.Uninstall();break;
   case "status":break;
   default:throw new InvalidDataException("Unknown fixture action.");
  }
  Console.WriteLine(JsonSerializer.Serialize(new{owner=e.Transition.Owner.ToString(),next=e.Transition.NextAction}));
 }
}
