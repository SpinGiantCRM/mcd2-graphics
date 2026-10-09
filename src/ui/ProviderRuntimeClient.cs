using NeoRune;
using UE.Engine;

// Volatile game-thread source applier. The actor supplies current world and
// measured console state; no persistent preference or GPU capability is inferred.
public class ProviderRuntimeClient : USaveGame {
 public ProviderSrContextSave? Context;
 public ProviderSrRuntimeSave? State;
 public int Sequence;public int Session;public int WorldGeneration;
 public string? Level;
 public bool Enabled;
 public static bool CanRestoreNvidiaSource(int savedRevision,int savedMode,int savedSession,int schema,int revision,int session,int phase){
  return savedRevision>0 && savedMode>0 && savedMode<=2 && savedSession>0 && schema==1 && revision==savedRevision && session==savedSession && phase==3;
 }
 public bool ContextValid(ProviderSrContextSave c){
  if(c.W0!=843334477 || c.W1!=827867987 || c.W2!=1 || c.W3!=128 || c.W4!=c.Checksum(0,32) ||
   c.W5<1 || c.W6<1 || c.W7<1 || c.W9<1 || c.W10<0 || c.W10>1 || c.W11<0 || c.W11>1000000000 ||
   (c.W10==1 && c.W11==0) || (c.W12<1000000 && (c.W10==1 || c.W12!=0)) || c.W12>100000000 || c.W13<0 || c.W15<0 || c.W15>2 ||
   (c.W13==0 && (c.W14!=0 || c.W15!=0)) || (c.W13>0 && (c.W14<1000000 || c.W14>100000000)))return false;
  for(int n=16;n<32;n++)if(c.Word(n)!=0)return false;
  return true;
 }
 public bool StateValid(ProviderSrRuntimeSave s){
  if(s.W0!=843334477 || s.W1!=827609939 || s.W2!=1 || s.W3!=128 || s.W4!=s.Checksum(0,32) ||
     s.W5<1 || s.W6<1 || s.W7<1 || s.W9<1 || s.W10<0 || s.W10>9 || s.W11<0 || s.W11>64 ||
     s.W16<1000000 || s.W16>100000000 || s.W17<0 || s.W18<0 || s.W18>3 || s.W19<0 || s.W19>5 ||
     s.W20<0 || s.W20>15)return false;
  for(int n=21;n<32;n++)if(s.Word(n)!=0)return false;
  if(s.W10>=2 && s.W10<=5 && (s.W12<640 || s.W13<360 || s.W12>s.W14 || s.W13>s.W15 ||
     s.W14>7680 || s.W15>4320))return false;
  return true;
 }
 public void Start(){
  Level="";
  var context=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsProviderSrContext",0) as ProviderSrContextSave;
  if(context==null){context=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderSrContextSave>()) as ProviderSrContextSave;
   if(context!=null)UGameplayStatics.SaveGameToSlot(context,"MCD2GraphicsProviderSrContext",0);}
  var state=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsProviderSrRuntime",0) as ProviderSrRuntimeSave;
  if(state==null){state=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderSrRuntimeSave>()) as ProviderSrRuntimeSave;
   if(state!=null)UGameplayStatics.SaveGameToSlot(state,"MCD2GraphicsProviderSrRuntime",0);}
 }
 public bool Poll(ProviderMenuClient menu,string level,bool ready,int worldToMetersMilli,int observedScaleMicro){
  Enabled=false;State=null;
  if(!menu.Ready || menu.Authority==null){
   // A lost settings authority may still finish an already admitted Native
   // rollback. This route can command only 100%, never activate a provider.
   var restore=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsProviderSrRuntime",0) as ProviderSrRuntimeSave;
   if(Context==null || restore==null || !StateValid(restore) || restore.W10!=7 || restore.W16!=100000000 ||
      restore.W5!=Session || restore.W9!=WorldGeneration || restore.W7!=Context.W7 || restore.W8!=Context.W8 || Sequence>=2147483646)return false;
   State=restore;Enabled=true;Context.W6=++Sequence;Context.W12=observedScaleMicro;Context.W4=Context.Checksum(0,32);
   return ContextValid(Context) && UGameplayStatics.SaveGameToSlot(Context,"MCD2GraphicsProviderSrContext",0);
  }
  int session=menu.Authority.W5;
  if(Session!=session){Session=session;Sequence=0;WorldGeneration=0;Level="";Context=null;
   var previous=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsProviderSrContext",0) as ProviderSrContextSave;
   if(previous!=null && ContextValid(previous) && previous.W5==session){Sequence=previous.W6;WorldGeneration=previous.W9;}
  }
  if(level!=Level || WorldGeneration==0){Level=level;WorldGeneration++;if(WorldGeneration<1)WorldGeneration=1;Context=null;}
  if((observedScaleMicro<1000000 && (ready || observedScaleMicro!=0)) || observedScaleMicro>100000000 || worldToMetersMilli<0 || worldToMetersMilli>1000000000)return false;
  var next=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsProviderSrRuntime",0) as ProviderSrRuntimeSave;
  if(next!=null && StateValid(next) && next.W5==session && next.W9==WorldGeneration && next.W7==menu.Authority.W19 && next.W8==menu.Authority.W16){State=next;Enabled=true;}
  if(Context==null)Context=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderSrContextSave>()) as ProviderSrContextSave;
  if(Context==null || Sequence>=2147483646)return false;
  Context.W0=843334477;Context.W1=827867987;Context.W2=1;Context.W3=128;Context.W5=session;
  Context.W6=++Sequence;Context.W7=menu.Authority.W19;Context.W8=menu.Authority.W16;Context.W9=WorldGeneration;
  Context.W10=ready && worldToMetersMilli>0?1:0;Context.W11=worldToMetersMilli;Context.W12=observedScaleMicro;
  // Reset acknowledgements on travel/session. A new plan is acknowledged only
  // after the actor observes its requested console value in this world.
  Context.W4=Context.Checksum(0,32);
  return UGameplayStatics.SaveGameToSlot(Context,"MCD2GraphicsProviderSrContext",0);
 }
 public bool NeedsSourceCommand(){
  if(!Enabled || State==null || Context==null || State.W5!=Session || State.W9!=WorldGeneration)return false;
  if(State.W10!=2 && State.W10!=7)return false;
  if(State.W10==2 && Context.W10!=1)return false;
  // Reapply when native game settings overwrite scale after the first ACK.
  int delta=Context.W12-State.W16;if(delta<0)delta=-delta;
  return Context.W13!=State.W6 || Context.W14!=State.W16 || delta>100;
 }
 public int SourceTarget(){return State==null?100000000:State.W16;}
 public void SourceObserved(int observedScaleMicro){
  if(State==null || Context==null)return;
  int delta=observedScaleMicro-State.W16;if(delta<0)delta=-delta;
  Context.W12=observedScaleMicro;Context.W13=State.W6;Context.W14=State.W16;Context.W15=delta<=100?0:1;
  Context.W4=Context.Checksum(0,32);UGameplayStatics.SaveGameToSlot(Context,"MCD2GraphicsProviderSrContext",0);
 }
 public void Stop(){if(Context!=null){Context.W10=0;Context.W13=0;Context.W14=0;Context.W15=0;
  if(Sequence<2147483646)Context.W6=++Sequence;Context.W4=Context.Checksum(0,32);
  UGameplayStatics.SaveGameToSlot(Context,"MCD2GraphicsProviderSrContext",0);}}
}
