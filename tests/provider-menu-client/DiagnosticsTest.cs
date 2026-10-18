using UE.Engine;
using MCD2Graphics;
namespace MCD2Graphics {
 // Test-only host facade; production ModActor consumes this exact client.
 public class ModActor {
  public ProviderMenuClient? ProviderMenu;public ProviderRuntimeClient? ProviderRuntime;
  public int Provider=1,FgProvider,FgMode=1;
  public bool DiagnosticSrActive,DiagnosticFgActive,DiagnosticReflexActive,DiagnosticAmdActive;
  public int SelectedProvider()=>Provider;public int SelectedFgProvider()=>FgProvider;public int DisplayValue(int id)=>FgMode;
 }
}
static class DiagnosticsTest {
 static void Need(bool ok){if(!ok)throw new Exception("Live diagnostics assertion failed");}
 static void Poll(RuntimeDiagnosticsClient c,ModActor m,bool enabled=true){c.Poll(m,enabled);m.DiagnosticSrActive=c.SrAdvancing;m.DiagnosticFgActive=c.FgAdvancing;m.DiagnosticReflexActive=c.ReflexAdvancing;m.DiagnosticAmdActive=c.AmdAdvancing;}
 public static void Run(){
  UGameplayStatics.Slots.Clear();var c=new RuntimeDiagnosticsClient();c.Start();
  var m=new ModActor{ProviderMenu=new ProviderMenuClient{Ready=true,Authority=new ProviderAuthoritySave{W5=123,W19=7}},ProviderRuntime=new ProviderRuntimeClient{WorldGeneration=4}};
  var s=new DiagnosticsStateSave{SchemaVersion=1,SessionId=123,Revision=7,Sample=1,WorldGeneration=4,SrProvider=1,SrFrames=100,FgProvider=1,FgActive=1,FgGenerated=10,ReflexMode=1,ReflexFrames=100};
  UGameplayStatics.Slots["MCD2GraphicsDiagnosticsState"]=s;Poll(c,m);
  Need(c.Fresh&&!c.SrAdvancing&&!c.FgAdvancing); // First sample proves no progression.
  s.Sample++;s.SrFrames++;s.FgGenerated++;s.ReflexFrames++;Poll(c,m);Need(c.SrAdvancing&&c.FgAdvancing&&c.ReflexAdvancing);
  Poll(c,m);Need(c.Fresh&&c.SrAdvancing); // Bounded retention between producer samples.
  s.Sample++;Poll(c,m);Need(c.Fresh&&!c.SrAdvancing&&!c.FgAdvancing&&!c.ReflexAdvancing);
  for(int i=0;i<4;i++)Poll(c,m);Need(!c.Fresh&&c.State==null); // Frozen producer expires.
  s.Sample++;s.SrFrames=0;s.FgGenerated=0;Poll(c,m);Need(!c.SrAdvancing&&!c.FgAdvancing); // Context reset is not progress.
  s.Sample++;s.SrFrames++;s.WorldGeneration=3;Poll(c,m);Need(!c.SrAdvancing);
  s.WorldGeneration=4;s.Sample++;s.SrFrames++;s.FgGenerated++;s.FgFault=1;Poll(c,m);Need(c.Fresh&&c.SrAdvancing&&!c.FgAdvancing); // FG fault cannot erase SR.
  s.FgFault=0;s.Sample++;s.SrFrames++;s.FgGenerated++;m.FgMode=0;Poll(c,m);Need(c.SrAdvancing&&!c.FgAdvancing);
  int currentSample=s.Sample;s.Sample--;Poll(c,m);Need(!c.Fresh);s.Sample=currentSample;
  s.SessionId=122;Poll(c,m);Need(!c.Fresh);s.SessionId=123;s.Revision=6;Poll(c,m);Need(!c.Fresh);
  s.Revision=7;s.Sample++;s.SrFrames++;Poll(c,m,false);Need(!c.Fresh&&c.State==null);
  var request=(DiagnosticsRequestSave)UGameplayStatics.Slots["MCD2GraphicsDiagnosticsRequest"];Need(request.Enabled==0&&request.SessionId==123);
  Need(RuntimeDiagnosticsClient.Implementation(30105)=="3.1.5"&&RuntimeDiagnosticsClient.Implementation(30106)=="3.1.6");
  Console.WriteLine("Live diagnostics: advancing counters, first sample, stale producer, stopped/reset counters, session/world/revision, isolated FG faults and opt-out checks pass");
 }
}
