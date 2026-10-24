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
  Poll(c,m);Need(!c.Fresh&&c.State==null); // The same frozen sample cannot recover freshness.
  s.Sample++;s.SrFrames=0;s.FgGenerated=0;Poll(c,m);Need(!c.SrAdvancing&&!c.FgAdvancing); // Context reset is not progress.
  s.Sample++;s.SrFrames++;s.WorldGeneration=3;Poll(c,m);Need(!c.SrAdvancing);
  s.WorldGeneration=4;s.Sample++;s.SrFrames++;s.FgGenerated++;s.FgFault=1;Poll(c,m);Need(c.Fresh&&!c.SrAdvancing&&!c.FgAdvancing); // Recovery establishes a new baseline.
  s.Sample++;s.SrFrames++;s.FgGenerated++;Poll(c,m);Need(c.Fresh&&c.SrAdvancing&&!c.FgAdvancing); // FG fault cannot erase SR.
  s.FgFault=0;s.Sample++;s.SrFrames++;s.FgGenerated++;m.FgMode=0;Poll(c,m);Need(c.SrAdvancing&&!c.FgAdvancing);
  int currentSample=s.Sample;s.Sample--;Poll(c,m);Need(!c.Fresh);s.Sample=currentSample;
  s.SessionId=122;Poll(c,m);Need(!c.Fresh);s.SessionId=123;s.Revision=6;Poll(c,m);Need(!c.Fresh);
  s.Revision=7;s.Sample++;s.SrFrames++;Poll(c,m,false);Need(!c.Fresh&&c.State==null);
  var request=(DiagnosticsRequestSave)UGameplayStatics.Slots["MCD2GraphicsDiagnosticsRequest"];Need(request.Enabled==0&&request.SessionId==123);
  Need(RuntimeDiagnosticsClient.Implementation(30105)=="3.1.5"&&RuntimeDiagnosticsClient.Implementation(30106)=="3.1.6");
  // Losing the current world/freshness after real FSR use must not claim that
  // FSR has never been used during this session. Keep activity fail-closed.
  var contextClient=new RuntimeDiagnosticsClient();
  var contextManager=new ModActor{Provider=2,ProviderMenu=m.ProviderMenu,ProviderRuntime=new ProviderRuntimeClient{WorldGeneration=4}};
  var contextState=new DiagnosticsStateSave{SchemaVersion=1,SessionId=123,Revision=7,Sample=200,WorldGeneration=4,SrProvider=2,SrFrames=20,FsrSrVersion=30105,FsrFgVersion=30106};
  UGameplayStatics.Slots["MCD2GraphicsDiagnosticsState"]=contextState;Poll(contextClient,contextManager);
  contextState.Sample++;contextState.SrFrames++;Poll(contextClient,contextManager);Need(contextClient.SrAdvancing);
  Need(RuntimeDiagnosticsClient.Implementation(contextClient.State!.FsrSrVersion)=="3.1.5");
  contextState.Sample++;contextState.WorldGeneration=0;Poll(contextClient,contextManager);
  Need(contextClient.State==null&&!contextClient.SrAdvancing&&!contextClient.FgAdvancing);
  if(RuntimeDiagnosticsClient.Implementation(contextClient.State==null?0:contextClient.State.FsrSrVersion)!="Not verified in current context")throw new Exception("Paused FSR diagnostics falsely deny prior use in this session");
  contextState.WorldGeneration=4;contextState.Sample++;Poll(contextClient,contextManager);
  for(int i=0;i<4;i++)Poll(contextClient,contextManager);
  Need(contextClient.State==null&&!contextClient.SrAdvancing);
  Need(RuntimeDiagnosticsClient.Implementation(0)=="Not verified in current context"&&RuntimeDiagnosticsClient.Implementation(-1)=="Not verified in current context");
  Poll(contextClient,contextManager,false);Need(contextClient.State==null&&!contextClient.FgAdvancing);
  UGameplayStatics.Slots["MCD2GraphicsDiagnosticsState"]=s;
  // Wrong/zero producer world must suppress every channel, including Native SR.
  m.Provider=0;m.FgMode=1;s.SrProvider=0;s.SrFrames=0;s.SessionId=123;s.Revision=7;s.WorldGeneration=4;s.AmdMode=1;s.AmdFrames=100;
  s.Sample++;Poll(c,m);s.Sample++;s.FgGenerated++;s.ReflexFrames++;s.AmdFrames++;Poll(c,m);
  Need(c.FgAdvancing&&c.ReflexAdvancing&&c.AmdAdvancing&&!c.SrAdvancing);
  s.Sample++;s.SrFrames=100;Poll(c,m);Need(!c.SrAdvancing); // Native never reports reconstruction activity.
  foreach(int wrongWorld in new[]{0,3}){s.WorldGeneration=wrongWorld;s.Sample++;s.FgGenerated++;s.ReflexFrames++;s.AmdFrames++;Poll(c,m);Need(!c.FgAdvancing&&!c.ReflexAdvancing&&!c.AmdAdvancing);}
  s.WorldGeneration=4;s.Sample++;s.FgGenerated++;s.ReflexFrames++;s.AmdFrames++;Poll(c,m);Need(!c.FgAdvancing&&!c.ReflexAdvancing&&!c.AmdAdvancing);
  s.Sample++;s.FgGenerated++;s.ReflexFrames++;s.AmdFrames++;Poll(c,m);Need(c.FgAdvancing&&c.ReflexAdvancing&&c.AmdAdvancing);
  // Turning diagnostics off or losing producer freshness invalidates the baseline.
  Poll(c,m,false);s.Sample++;s.FgGenerated+=10;s.ReflexFrames+=10;s.AmdFrames+=10;Poll(c,m);Need(!c.FgAdvancing&&!c.ReflexAdvancing&&!c.AmdAdvancing);
  s.Sample++;s.FgGenerated++;s.ReflexFrames++;s.AmdFrames++;Poll(c,m);Need(c.FgAdvancing&&c.ReflexAdvancing&&c.AmdAdvancing);
  for(int i=0;i<4;i++)Poll(c,m);Need(!c.Fresh);
  s.Sample++;s.FgGenerated+=10;s.ReflexFrames+=10;s.AmdFrames+=10;Poll(c,m);Need(!c.FgAdvancing&&!c.ReflexAdvancing&&!c.AmdAdvancing);
  // An unavailable SR read is not a counter reset; other channels stay independent.
  var isolated=new RuntimeDiagnosticsClient();m.Provider=2;m.FgProvider=0;m.FgMode=1;
  s.SrProvider=2;s.SrFrames=100;s.FgProvider=1;s.FgActive=1;s.FgFault=0;s.AmdMode=1;
  s.Sample++;Poll(isolated,m);Need(!isolated.SrAdvancing);
  s.Sample++;s.SrProvider=0;s.SrFrames=0;s.FgGenerated++;s.AmdFrames++;Poll(isolated,m);
  Need(!isolated.SrAdvancing&&isolated.FgAdvancing&&isolated.AmdAdvancing);
  s.Sample++;s.SrProvider=2;s.SrFrames=100;s.FgGenerated++;s.AmdFrames++;Poll(isolated,m);
  Need(!isolated.SrAdvancing&&isolated.FgAdvancing&&isolated.AmdAdvancing); // 100 -> Busy -> unchanged 100
  s.Sample++;s.SrFrames=101;Poll(isolated,m);Need(isolated.SrAdvancing);
  s.Sample++;s.SrFrames=3;Poll(isolated,m);Need(!isolated.SrAdvancing); // A real reset never advances.
  s.Sample++;s.SrFrames=4;Poll(isolated,m);Need(isolated.SrAdvancing);
  // Each independent provider must establish a new baseline after its own unavailable/Off sample.
  for(int channel=0;channel<3;channel++){
   var independent=new RuntimeDiagnosticsClient();m.Provider=2;m.FgProvider=0;m.FgMode=1;
   s.SrProvider=2;s.SrFrames=100;s.FgProvider=1;s.FgActive=1;s.FgGenerated=100;s.ReflexMode=1;s.ReflexFrames=100;s.AmdMode=1;s.AmdFrames=100;
   s.Sample++;Poll(independent,m);s.Sample++;s.SrFrames++;s.FgGenerated++;s.ReflexFrames++;s.AmdFrames++;Poll(independent,m);
   Need(independent.SrAdvancing&&independent.FgAdvancing&&independent.ReflexAdvancing&&independent.AmdAdvancing);
   s.Sample++;s.SrFrames++;s.FgGenerated++;s.ReflexFrames++;s.AmdFrames++;
   if(channel==0){s.FgProvider=0;s.FgActive=0;s.FgGenerated=0;}if(channel==1){s.ReflexMode=0;s.ReflexFrames=0;}if(channel==2){s.AmdMode=0;s.AmdFrames=0;}
   Poll(independent,m);Need(independent.SrAdvancing&&independent.FgAdvancing==(channel!=0)&&independent.ReflexAdvancing==(channel!=1)&&independent.AmdAdvancing==(channel!=2));
   s.Sample++;s.SrFrames++;s.FgGenerated++;s.ReflexFrames++;s.AmdFrames++;
   if(channel==0){s.FgProvider=1;s.FgActive=1;s.FgGenerated=101;}if(channel==1){s.ReflexMode=1;s.ReflexFrames=101;}if(channel==2){s.AmdMode=1;s.AmdFrames=101;}
   Poll(independent,m);Need(independent.SrAdvancing&&independent.FgAdvancing==(channel!=0)&&independent.ReflexAdvancing==(channel!=1)&&independent.AmdAdvancing==(channel!=2)); // Unchanged recovery total is no progress.
   s.Sample++;s.SrFrames++;s.FgGenerated++;s.ReflexFrames++;s.AmdFrames++;Poll(independent,m);Need(independent.SrAdvancing&&independent.FgAdvancing&&independent.ReflexAdvancing&&independent.AmdAdvancing);
   if(channel==0)m.FgMode=0;if(channel==1)s.ReflexMode=0;if(channel==2)s.AmdMode=0;
   Poll(independent,m);Need(independent.SrAdvancing&&independent.FgAdvancing==(channel!=0)&&independent.ReflexAdvancing==(channel!=1)&&independent.AmdAdvancing==(channel!=2)); // Off is immediate between producer samples.
  }
  // Only an explicit native Busy reuse may retain previous DLSS progression.
  var retainedClient=new RuntimeDiagnosticsClient();m.Provider=1;m.FgProvider=0;m.FgMode=1;
  s.SrProvider=1;s.SrFrames=100;s.SrRetained=0;s.FgActive=1;s.FgProvider=1;s.FgFault=0;
  s.Sample++;Poll(retainedClient,m);s.Sample++;s.SrFrames++;Poll(retainedClient,m);Need(retainedClient.SrAdvancing);
  s.Sample++;s.SrRetained=1;s.FgGenerated++;Poll(retainedClient,m);Need(retainedClient.SrAdvancing&&retainedClient.FgAdvancing);
  s.Sample++;Poll(retainedClient,m);Need(retainedClient.SrAdvancing); // Original native observation is still within its bound.
  s.Sample++;s.SrRetained=0;Poll(retainedClient,m);Need(!retainedClient.SrAdvancing); // Successful unchanged read is no progress.
  s.Sample++;s.SrRetained=1;Poll(retainedClient,m);Need(!retainedClient.SrAdvancing); // Busy cannot invent prior progression.
  s.Sample++;s.SrRetained=0;s.SrFrames++;Poll(retainedClient,m);Need(retainedClient.SrAdvancing);
  s.Sample++;s.SrRetained=1;s.SrFrames++;Poll(retainedClient,m);Need(!retainedClient.SrAdvancing); // Retention cannot claim new evaluations.
  s.Sample++;s.SrRetained=0;s.SrFrames++;Poll(retainedClient,m);Need(retainedClient.SrAdvancing);
  s.Sample++;s.SrProvider=0;s.SrFrames=0;Poll(retainedClient,m);Need(!retainedClient.SrAdvancing); // Native Busy expiry is immediate.
  s.Sample++;s.SrProvider=1;s.SrFrames=104;Poll(retainedClient,m);Need(!retainedClient.SrAdvancing); // Recovery needs a baseline.
  s.Sample++;s.SrFrames++;Poll(retainedClient,m);Need(retainedClient.SrAdvancing);
  s.Sample++;s.SrRetained=1;m.Provider=0;Poll(retainedClient,m);Need(!retainedClient.SrAdvancing);
  m.Provider=1;s.Sample++;Poll(retainedClient,m);Need(!retainedClient.SrAdvancing); // Retained after Off cannot recover activity.
  s.Sample++;s.SrRetained=0;s.SrFrames++;Poll(retainedClient,m);Need(retainedClient.SrAdvancing);
  s.Sample++;s.SrRetained=1;Poll(retainedClient,m);Need(retainedClient.SrAdvancing);
  for(int i=0;i<4;i++)Poll(retainedClient,m);Need(!retainedClient.Fresh&&!retainedClient.SrAdvancing); // Frozen producer still expires.
  s.Sample++;s.SrRetained=2;Poll(retainedClient,m);Need(!retainedClient.Fresh); // Provenance is a bounded Boolean.
  s.Sample++;s.SrRetained=1;s.SrProvider=2;Poll(retainedClient,m);Need(!retainedClient.Fresh); // FSR activity never comes from this getter.
  s.SrRetained=0;s.SrProvider=2;
  // Separate components require actual advancing presents under the current strategy.
  var mixedClient=new RuntimeDiagnosticsClient();m.FgMode=1;m.FgProvider=0;m.ProviderMenu!.Authority!.W33=2;
  s.FgProvider=1;s.FgActive=1;s.FgFault=0;s.FgStrategy=2;s.FgSoftwareGenerated=10;s.FgNativeGenerated=5;
  s.Sample++;Poll(mixedClient,m);Need(!mixedClient.SoftwareFgAdvancing&&!mixedClient.NativeFgAdvancing);
  s.Sample++;s.FgGenerated+=3;s.FgSoftwareGenerated+=2;s.FgNativeGenerated++;Poll(mixedClient,m);
  Need(mixedClient.SoftwareFgAdvancing&&mixedClient.NativeFgAdvancing);
  Poll(mixedClient,m);Need(mixedClient.SoftwareFgAdvancing&&mixedClient.NativeFgAdvancing);
  s.Sample++;s.FgGenerated++;s.FgSoftwareGenerated++;Poll(mixedClient,m);
  Need(mixedClient.SoftwareFgAdvancing&&!mixedClient.NativeFgAdvancing);
  m.ProviderMenu.Authority.W33=3;Poll(mixedClient,m);Need(!mixedClient.SoftwareFgAdvancing&&!mixedClient.NativeFgAdvancing);
  m.ProviderMenu.Authority.W33=2;s.Sample++;s.FgSoftwareGenerated++;s.FgNativeGenerated++;Poll(mixedClient,m);
  Need(!mixedClient.SoftwareFgAdvancing&&!mixedClient.NativeFgAdvancing); // Fresh baseline after mismatch.
  s.Sample++;s.FgGenerated+=3;s.FgSoftwareGenerated+=2;s.FgNativeGenerated++;Poll(mixedClient,m);
  Need(mixedClient.SoftwareFgAdvancing&&mixedClient.NativeFgAdvancing);
  m.FgMode=0;Poll(mixedClient,m);Need(!mixedClient.SoftwareFgAdvancing&&!mixedClient.NativeFgAdvancing);
  Console.WriteLine("Live diagnostics: advancing counters, Native independence, all-channel world validation and invalid/stale/opt-out baseline recovery pass");
 }
}
