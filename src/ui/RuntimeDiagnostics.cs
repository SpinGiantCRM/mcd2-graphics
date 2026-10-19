using NeoRune;
using MCD2Graphics;
using UE.CoreUObject;
using UE.Engine;

public class DiagnosticsRequestSave : USaveGame {public int SchemaVersion,SessionId,Enabled;}
public class DiagnosticsStateSave : USaveGame {
 public int SchemaVersion,SessionId,Revision,Sample,WorldGeneration,SrProvider,SrFrames,FsrSrVersion;
 public int FgProvider,FgActive,FgFault,FgRendered,FgGenerated,FsrFgVersion,ReflexMode,ReflexFrames,AmdMode,AmdFrames,FgPresetStatus,FsrHardware;
}
public class RuntimeDiagnosticsClient : USaveGame {
 public DiagnosticsStateSave? State;
 public bool Fresh,SrAdvancing,FgAdvancing,ReflexAdvancing,AmdAdvancing;
 int sample,age,lastSr,lastSrProvider,lastFg,lastFgProvider,lastReflex,lastReflexMode,lastAmd,lastAmdMode,lastSession,lastRevision,lastWorld;
 int producerSession,producerRevision,producerWorld,producerSample;bool requireNewSample;
 void Invalidate(){State=null;sample=0;age=0;lastSr=lastSrProvider=lastFg=lastFgProvider=lastReflex=lastReflexMode=lastAmd=lastAmdMode=0;lastSession=lastRevision=lastWorld=0;requireNewSample=true;}
 public void Start(){
  var state=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<DiagnosticsStateSave>()) as DiagnosticsStateSave;
  if(state!=null){state.SchemaVersion=1;UGameplayStatics.SaveGameToSlot(state,"MCD2GraphicsDiagnosticsState",0);}
 }
 public void Poll(ModActor manager,bool enabled){
  Fresh=false;SrAdvancing=false;FgAdvancing=false;ReflexAdvancing=false;AmdAdvancing=false;
  var menu=manager.ProviderMenu;int session=menu!=null&&menu.Ready&&menu.Authority!=null?menu.Authority.W5:0;
  var request=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsDiagnosticsRequest",0) as DiagnosticsRequestSave;
  if(request==null)request=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<DiagnosticsRequestSave>()) as DiagnosticsRequestSave;
  if(request!=null && (request.SchemaVersion!=1||request.SessionId!=session||request.Enabled!=(enabled?1:0))){request.SchemaVersion=1;request.SessionId=session;request.Enabled=enabled?1:0;UGameplayStatics.SaveGameToSlot(request,"MCD2GraphicsDiagnosticsRequest",0);}
  if(!enabled||session==0||menu==null||menu.Authority==null){Invalidate();return;}
  var next=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsDiagnosticsState",0) as DiagnosticsStateSave;
  if(next==null||next.SchemaVersion!=1||next.SessionId!=session||next.Revision!=menu.Authority.W19||next.Sample<1||next.WorldGeneration<0||next.SrFrames<0||next.FgRendered<0||next.FgGenerated<0||next.ReflexFrames<0||next.AmdFrames<0||next.FgActive<0||next.FgActive>1||
     next.FsrHardware<0||next.FsrHardware>3||next.FsrHardware==2||next.SrProvider<0||next.SrProvider>2||next.FgProvider<0||next.FgProvider>2||next.FgFault<0||next.ReflexMode<0||next.ReflexMode>2||next.AmdMode<0||next.AmdMode>1){Invalidate();return;}
  int world=manager.ProviderRuntime==null?0:manager.ProviderRuntime.WorldGeneration;
  if(world<1||next.WorldGeneration!=world){Invalidate();return;}
  bool sameProducer=producerSession==session&&producerRevision==next.Revision&&producerWorld==world;
  if(sameProducer&&(next.Sample<producerSample||(requireNewSample&&next.Sample<=producerSample))){Invalidate();return;}
  producerSession=session;producerRevision=next.Revision;producerWorld=world;producerSample=next.Sample;requireNewSample=false;
  bool srEligible=next.SrProvider>0&&next.SrProvider==manager.SelectedProvider();
  bool fgEligible=next.FgActive==1&&next.FgFault==0&&next.FgProvider==(manager.SelectedFgProvider()==1?2:1)&&manager.DisplayValue(5)==1;
  bool reflexEligible=next.ReflexMode>0,amdEligible=next.AmdMode==1;
  if(!srEligible)lastSrProvider=0;if(!fgEligible)lastFgProvider=0;if(!reflexEligible)lastReflexMode=0;if(!amdEligible)lastAmdMode=0;
  bool same=lastSession==session&&lastRevision==next.Revision&&lastWorld==world;
  if(!same){sample=0;age=0;}
  if(same&&next.Sample<sample){Invalidate();return;}
  if(next.Sample==sample){age++;if(age>3){Invalidate();return;}}
  else{
   SrAdvancing=same&&next.SrFrames>lastSr&&srEligible&&next.SrProvider==lastSrProvider;
   lastSrProvider=srEligible?next.SrProvider:0;
   FgAdvancing=same&&fgEligible&&next.FgProvider==lastFgProvider&&next.FgGenerated>lastFg;
   lastFgProvider=fgEligible?next.FgProvider:0;
   ReflexAdvancing=same&&reflexEligible&&next.ReflexMode==lastReflexMode&&next.ReflexFrames>lastReflex;
   lastReflexMode=reflexEligible?next.ReflexMode:0;
   AmdAdvancing=same&&amdEligible&&lastAmdMode==1&&next.AmdFrames>lastAmd;
   lastAmdMode=amdEligible?1:0;
   sample=next.Sample;age=0;lastSr=next.SrFrames;lastFg=next.FgGenerated;lastReflex=next.ReflexFrames;lastAmd=next.AmdFrames;
  }
  // Retain the last observation between native samples, with bounded expiry.
  if(State!=null&&same&&age>0){SrAdvancing=srEligible&&lastSrProvider==next.SrProvider&&manager.DiagnosticSrActive;FgAdvancing=fgEligible&&lastFgProvider==next.FgProvider&&manager.DiagnosticFgActive;ReflexAdvancing=reflexEligible&&lastReflexMode==next.ReflexMode&&manager.DiagnosticReflexActive;AmdAdvancing=amdEligible&&lastAmdMode==1&&manager.DiagnosticAmdActive;}
  lastSession=session;lastRevision=next.Revision;lastWorld=world;State=next;Fresh=true;
 }
 public static string Implementation(int version){return version<=0?"Not used this session":(version/10000)+"."+((version/100)%100)+"."+(version%100);}
}
