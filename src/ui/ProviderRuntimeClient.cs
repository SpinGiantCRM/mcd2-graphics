using NeoRune;
using UE.Engine;

public class FGSettingsSave : USaveGame {public int SchemaVersion;public int Revision;public int Mode;public int ContextReady;public int SessionId;public int FGProvider;public int SettingsRevision;}
public class FGRuntimeSave : USaveGame {public int SchemaVersion;public int Revision;public int SessionId;public int Available;public int Active;public int Phase;public int RestartRequired;public int AvailableProviders;public int StartupOwner;public int PresenterRoute;public int PresenterCanSwitch;public int PresenterSettingsRevision;public int NativeGuidesRetired;}

// Volatile game-thread source applier. The actor supplies current world and
// measured console state; no persistent preference or GPU capability is inferred.
public class ProviderRuntimeClient : USaveGame {
 public ProviderSrContextSave? Context;
 public ProviderSrRuntimeSave? State;
 public int Sequence;public int Session;public int WorldGeneration;
 public string? Level;
 public bool Enabled;
 // Vendor compatibility comes from the current rendering device, separately
 // from FG/Reflex support. NVIDIA's backend still validates its SR capability.
 public static bool SrChoiceAvailable(int provider,bool ready,int session,int schema,int runtimeSession,int vendor){
  if(provider==0 || provider==2)return true;
  return provider==1 && ready && session>0 && schema==1 && runtimeSession==session && vendor==4318;
 }
 // Display the selected preset even before gameplay/preflight. Only the native
 // SDK plan authorizes a source-resolution change; this is a slider value.
 public static int PresetScale(int family,int quality,int custom){
  if(quality==0)return 10000;if(quality==5)return custom;
  return quality==1?6700:quality==2?(family==2?5900:5800):quality==3?5000:3300;
 }
 // Acquire the validated single-output TAA producer for SR or Native FG preflight.
 // This does not acknowledge source scale or authorize SDK evaluation.
 public bool TemporalShaderPreflight(ProviderMenuClient menu,FGSettingsSave? fgSettings=null,FGRuntimeSave? fgRuntime=null){
  if(!menu.Ready || menu.Authority==null || !Enabled || Context==null || State==null ||
     !ContextValid(Context) || !StateValid(State))return false;
  var a=menu.Authority;var c=Context;var s=State;
  bool matching=Session==a.W5 && c.W5==Session && s.W5==Session &&
   c.W9==WorldGeneration && s.W9==WorldGeneration &&
   c.W7==a.W19 && s.W7==a.W19 && c.W8==a.W16 && s.W8==a.W16;
  if(!matching || c.W10!=1 || s.W11!=0)return false;
  if(a.W20==2)return s.W18==2 && s.W10==1;
  return NativeTemporalRequest(menu) && s.W18==0 && (s.W10==0 || s.W10==8) &&
   s.W16==100000000 && c.W12==100000000 && c.W11==100000 &&
   FgTemporalAck(a,fgSettings,fgRuntime) && fgSettings!.ContextReady==1 &&
   fgRuntime!.Available==1 && (fgRuntime.AvailableProviders&2)!=0 && fgRuntime.AvailableProviders<=3 &&
   fgRuntime.StartupOwner==2 && fgRuntime.PresenterRoute==2 && fgRuntime.RestartRequired==0 &&
   (fgRuntime.Phase==2 || fgRuntime.Phase==3);
 }
 // This intent check retains an existing lease through pause and actor travel;
 // acquisition still needs the independently validated current-world snapshots.
 public bool NativeTemporalRequest(ProviderMenuClient menu){
  var a=menu.Authority;
  return menu.Ready && a!=null && a.W20==0 && a.W31==1 && a.W32==1 && a.W33==0 && a.W34==2;
 }
 static bool FgTemporalAck(ProviderAuthoritySave a,FGSettingsSave? settings,FGRuntimeSave? runtime){
  return settings!=null && runtime!=null && settings.SchemaVersion==1 && runtime.SchemaVersion==1 &&
   settings.Revision>0 && runtime.Revision==settings.Revision && runtime.SessionId>0 && runtime.SessionId==settings.SessionId &&
   settings.SettingsRevision==a.W19 && runtime.PresenterSettingsRevision==a.W19 &&
   settings.Mode==a.W31 && settings.FGProvider==a.W32 && runtime.Phase>=0 && runtime.Phase<=3 &&
   runtime.Active>=0 && runtime.Active<=1 && runtime.NativeGuidesRetired>=0 && runtime.NativeGuidesRetired<=1;
 }
 public bool TemporalQualityRetain(ProviderMenuClient menu,FGSettingsSave? settings,FGRuntimeSave? runtime,bool nativeLease){
  if(!menu.Ready || menu.Authority==null)return true;
  if(nativeLease && (NativeTemporalRequest(menu) || !FgTemporalAck(menu.Authority,settings,runtime) ||
     settings!.Mode!=0 || runtime!.Active!=0 || runtime.NativeGuidesRetired!=1))return true;
  var state=State;var context=Context;var a=menu.Authority;
  if(nativeLease && (!Enabled || context==null || state==null || !ContextValid(context) || !StateValid(state) ||
     Session!=a.W5 || context.W5!=Session || state.W5!=Session || context.W9!=WorldGeneration || state.W9!=WorldGeneration ||
     context.W7!=a.W19 || state.W7!=a.W19 || context.W8!=a.W16 || state.W8!=a.W16))return true;
  return state==null || (state.W10>=1 && state.W10<=6) || state.W10==9;
 }
 public static bool GameplayWidgetAllowed(string name,bool layer){
  if(name.StartsWith("W_PlayerHUD_Activatable_C_") || name.StartsWith("W_HotbarContainer_Activatable_C_") || name.StartsWith("W_InGameNavigation_Activatable_C_"))return true;
  return !layer && (name=="Hotbar" || name=="MiniInventory" || name.StartsWith("W_ActivityLayer_Activatable_C_") || name.Contains("Notification_Activatable_C_"));
 }
 public static bool AmdLatencyAvailable(bool authorityReady,int authoritySession,int runtimeSchema,int runtimeSession,int available,int fault,bool fgEnabled,int fgProvider=0,int fgCompatible=0){
  return authorityReady && authoritySession>0 && runtimeSchema==1 && runtimeSession==authoritySession && available==1 && fault==0 &&
   (!fgEnabled || (fgProvider==1 && fgCompatible==1));
 }
 public static int AmdLatencyApplied(int authorityRevision,int runtimeRevision,int runtimeMode){
  return authorityRevision>0 && runtimeRevision==authorityRevision && (runtimeMode==0 || runtimeMode==1)?runtimeMode:-1;
 }
 // A saved FSR selection cannot acquire a source-material override. Require
 // current-world evaluated output and the observed source scale; transient
 // Evaluate may retain an existing lease but cannot acquire a new one.
 public bool FoliageMotionReady(ProviderMenuClient menu,bool acquiring){
  if(!menu.Ready || menu.Authority==null || !Enabled || Context==null || State==null ||
     !ContextValid(Context) || !StateValid(State))return false;
  var a=menu.Authority;var c=Context;var s=State;
  int delta=c.W12-s.W16;if(delta<0)delta=-delta;
  // A temporary menu can retain an existing velocity lease while SR retires.
  // Acquisition still requires gameplay and active output; errors cannot retain it.
  bool pausedRetirement=!acquiring && c.W10==0 && s.W10==6;
  return a.W20==2 && (c.W10==1 || !acquiring) && s.W18==2 && s.W11==0 && s.W17>0 &&
   (s.W10==5 || (!acquiring && s.W10==4) || pausedRetirement) &&
   Session==a.W5 && c.W5==Session && s.W5==Session &&
   c.W9==WorldGeneration && s.W9==WorldGeneration &&
   c.W7==a.W19 && s.W7==a.W19 && c.W8==a.W16 && s.W8==a.W16 && delta<=100;
 }
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

// Volatile ownership receipt survives actor travel, but a new renderer session
// cannot adopt the old process's lease. Only the observed owned value is restored.
public class TemporalAaQualityLeaseSave : USaveGame {
 public int SchemaVersion,Session,Original,Owned,Blocked,Revision,Checksum,RestoreAttempts,NativeGuides;
 public int Command(int session,int revision,int checksum,bool exists,int observed,bool acquire,bool retain){
  if(session<=0 || revision<=0)return -1;
  if(SchemaVersion!=1 || Session!=session){SchemaVersion=1;Session=session;Owned=0;Blocked=0;RestoreAttempts=0;NativeGuides=0;}
  if(Revision!=revision || Checksum!=checksum){Revision=revision;Checksum=checksum;Blocked=0;}
  if(Owned==1){
   if(!exists || observed!=2){Owned=0;Blocked=1;return -1;}
   if(retain || RestoreAttempts>=3)return -1;
   RestoreAttempts++;return Original;
  }
  if(!acquire || Blocked==1 || !exists || observed<0 || observed>1)return -1;
  Original=observed;RestoreAttempts=0;return 2;
 }
 public void Observed(int command,int value){
  if(command==2){Owned=value==2?1:0;if(Owned==0)Blocked=1;}
  else if(value==Original)Owned=0;
 }
}
