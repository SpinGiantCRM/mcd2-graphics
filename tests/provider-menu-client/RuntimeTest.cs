using UE.Engine;
static class RuntimeTest {
 static void Check(bool value){if(!value)throw new Exception("Source runtime assertion failed");}
 static ProviderMenuClient Menu(){
  var menu=new ProviderMenuClient();var a=new ProviderAuthoritySave();a.W5=123;a.W19=7;a.W16=unchecked((int)0xdeadbeef);
  menu.Authority=a;menu.Ready=true;return menu;
 }
 static ProviderSrRuntimeSave Plan(int sequence,int phase=2){
  var s=new ProviderSrRuntimeSave();s.W0=0x3244434d;s.W1=0x31545353;s.W2=1;s.W3=128;s.W5=123;s.W6=sequence;s.W7=7;
  s.W8=unchecked((int)0xdeadbeef);s.W9=1;s.W10=phase;s.W12=2560;s.W13=1440;s.W14=3840;s.W15=2160;
  s.W16=66666667;s.W18=2;s.W19=1;s.W20=1;s.W4=s.Checksum(0,32);return s;
 }
 public static void Run(){
  foreach(int vendor in new[]{0,4098,32902}){
   Check(!ProviderRuntimeClient.SrChoiceAvailable(1,true,123,1,123,vendor));
   Check(ProviderRuntimeClient.SrChoiceAvailable(0,true,123,1,123,vendor));
   Check(ProviderRuntimeClient.SrChoiceAvailable(2,true,123,1,123,vendor));
  }
  Check(ProviderRuntimeClient.SrChoiceAvailable(1,true,123,1,123,4318));
  Check(!ProviderRuntimeClient.SrChoiceAvailable(1,true,123,1,122,4318));
  Check(!ProviderRuntimeClient.SrChoiceAvailable(1,false,123,1,123,4318));
  Check(!ProviderRuntimeClient.SrChoiceAvailable(1,true,123,0,123,4318));
  // The preset remains numeric in menus, before a runtime plan exists.
  foreach(int family in new[]{1,2}){
   int[] expected={10000,6700,family==2?5900:5800,5000,3300,7700};
   for(int quality=0;quality<expected.Length;quality++)Check(ProviderRuntimeClient.PresetScale(family,quality,7700)==expected[quality]);
  }
  ReadinessChecks();
  FoliageChecks();
  TemporalQualityChecks();
  Check(ProviderRuntimeClient.CanRestoreNvidiaSource(7,2,123,1,7,123,3));
  Check(!ProviderRuntimeClient.CanRestoreNvidiaSource(7,2,123,1,7,124,3));
  Check(!ProviderRuntimeClient.CanRestoreNvidiaSource(7,2,123,1,8,123,3));
  Check(!ProviderRuntimeClient.CanRestoreNvidiaSource(7,2,123,1,7,123,1));
  Check(!ProviderRuntimeClient.CanRestoreNvidiaSource(7,0,123,1,7,123,3));
  Check(!ProviderRuntimeClient.CanRestoreNvidiaSource(7,2,0,1,7,123,3));
  Check(ProviderRuntimeClient.AmdLatencyAvailable(true,123,1,123,1,0,false));
  Check(!ProviderRuntimeClient.AmdLatencyAvailable(false,123,1,123,1,0,false));
  Check(!ProviderRuntimeClient.AmdLatencyAvailable(true,123,1,124,1,0,false));
  Check(!ProviderRuntimeClient.AmdLatencyAvailable(true,123,2,123,1,0,false));
  Check(!ProviderRuntimeClient.AmdLatencyAvailable(true,123,1,123,0,0,false));
  Check(!ProviderRuntimeClient.AmdLatencyAvailable(true,123,1,123,1,1,false));
  Check(!ProviderRuntimeClient.AmdLatencyAvailable(true,123,1,123,1,0,true));
  Check(ProviderRuntimeClient.AmdLatencyAvailable(true,123,1,123,1,0,true,1,1));
  Check(!ProviderRuntimeClient.AmdLatencyAvailable(true,123,1,124,1,0,true,1,1));
  Check(!ProviderRuntimeClient.AmdLatencyAvailable(true,123,1,123,1,0,true,0,1));
  Check(!ProviderRuntimeClient.AmdLatencyAvailable(true,123,1,123,1,0,true,1,0));
  Check(!ProviderRuntimeClient.AmdLatencyAvailable(true,123,1,123,1,0,true,1,2));
  Check(ProviderRuntimeClient.AmdLatencyApplied(7,7,0)==0);
  Check(ProviderRuntimeClient.AmdLatencyApplied(7,7,1)==1);
  Check(ProviderRuntimeClient.AmdLatencyApplied(7,6,1)==-1);
  Check(ProviderRuntimeClient.AmdLatencyApplied(7,7,2)==-1);
  UGameplayStatics.Slots.Clear();UGameplayStatics.FailSave=false;var menu=Menu();var client=new ProviderRuntimeClient();client.Start();
  var s=Plan(5);UGameplayStatics.Slots["MCD2GraphicsProviderSrRuntime"]=s;
  Check(client.Poll(menu,"Hub",true,100000,100000000)&&client.Enabled&&client.NeedsSourceCommand());
  Check(client.SourceTarget()==66666667);client.SourceObserved(66666668);Check(!client.NeedsSourceCommand());
  Check(client.Context!.W13==5&&client.Context.W15==0&&client.Context.W1==0x31584353&&client.ContextValid(client.Context));
  Check(client.Poll(menu,"Hub",true,100000,100000000)&&client.NeedsSourceCommand()); // Engine settings reapply.
  s.W9=2;s.W10=7;s.W16=100000000;s.W6=6;s.W4=s.Checksum(0,32);
  Check(client.Poll(menu,"Menu",false,100000,66666667)&&client.NeedsSourceCommand()); // Rollback works outside gameplay.
  menu.Ready=false;Check(client.Poll(menu,"Menu",false,100000,66666667)&&client.Enabled&&client.NeedsSourceCommand());
  client.SourceObserved(100000000);Check(!client.NeedsSourceCommand());Check(client.Context.W10==0);menu.Ready=true;
  int previous=client.Sequence,world=client.WorldGeneration;var travel=new ProviderRuntimeClient();travel.Start();
  Check(travel.Poll(menu,"Dungeon",true,100000,100000000)&&travel.Sequence>previous&&travel.WorldGeneration>world);
  Check(travel.Context!.W13==0&&travel.Context.W14==0); // Never adopt the old world's source ACK.
  s.W9=travel.WorldGeneration-1;s.W4=s.Checksum(0,32);Check(travel.Poll(menu,"Dungeon",true,100000,100000000)&&!travel.Enabled); // stale previous actor/world never admits controls
  s.W9=travel.WorldGeneration;s.W5=124;s.W4=s.Checksum(0,32);Check(travel.Poll(menu,"Dungeon",true,100000,100000000)&&!travel.Enabled);
  s.W5=123;s.W7=8;s.W4=s.Checksum(0,32);Check(travel.Poll(menu,"Dungeon",true,100000,100000000)&&!travel.Enabled);
  s.W7=7;s.W4=s.Checksum(0,32);s.W4^=1;Check(travel.Poll(menu,"Dungeon",true,100000,100000000)&&!travel.Enabled);
  s.W4^=1;s.W21=1;s.W4=s.Checksum(0,32);Check(!travel.StateValid(s));s.W21=0;s.W4=s.Checksum(0,32);
  s.W10=2;s.W12=639;s.W4=s.Checksum(0,32);Check(!travel.StateValid(s));
  UGameplayStatics.FailSave=true;Check(!travel.Poll(menu,"Dungeon",true,100000,100000000));UGameplayStatics.FailSave=false;
  travel.Stop();Check(travel.Context.W10==0&&travel.Context.W13==0&&travel.Context.W4==travel.Context.Checksum(0,32));
  Console.WriteLine("SR game-thread client: exact scale, reapply, rollback, actor travel, stale plan and corruption checks pass");
 }
 static void ReadinessChecks(){
  foreach(string name in new[]{"W_PlayerHUD_Activatable_C_1","W_HotbarContainer_Activatable_C_4","W_InGameNavigation_Activatable_C_9"}){
   Check(ProviderRuntimeClient.GameplayWidgetAllowed(name,true));Check(ProviderRuntimeClient.GameplayWidgetAllowed(name,false));
  }
  foreach(string name in new[]{"Hotbar","MiniInventory","W_ActivityLayer_Activatable_C_2","W_LootNotification_Activatable_C_3"}){
   Check(!ProviderRuntimeClient.GameplayWidgetAllowed(name,true));Check(ProviderRuntimeClient.GameplayWidgetAllowed(name,false));
  }
  foreach(string name in new[]{"","Activity_Stack","W_Settings_Activatable_C_1","W_System_Activatable_C_2","W_Inventory_Activatable_C_3","W_Map_Activatable_C_4","W_UnknownMod_Activatable_C_5"}){
   Check(!ProviderRuntimeClient.GameplayWidgetAllowed(name,true));Check(!ProviderRuntimeClient.GameplayWidgetAllowed(name,false));
  }
  Console.WriteLine("28 gameplay widget admission checks passed");
 }
 static void FoliageChecks(){
  UGameplayStatics.Slots.Clear();var menu=Menu();menu.Authority!.W20=2;
  var client=new ProviderRuntimeClient();client.Start();var s=Plan(5,5);s.W17=1;s.W4=s.Checksum(0,32);
  UGameplayStatics.Slots["MCD2GraphicsProviderSrRuntime"]=s;
  Check(client.Poll(menu,"Hub",true,100000,66666667));
  Check(client.FoliageMotionReady(menu,true)&&client.FoliageMotionReady(menu,false));
  var c=client.Context!;
  c.W10=0;c.W4=c.Checksum(0,32);Check(!client.FoliageMotionReady(menu,true)&&client.FoliageMotionReady(menu,false));
  s.W10=6;s.W4=s.Checksum(0,32);Check(!client.FoliageMotionReady(menu,true)&&client.FoliageMotionReady(menu,false));
  s.W11=22;s.W4=s.Checksum(0,32);Check(!client.FoliageMotionReady(menu,false));
  s.W11=0;s.W10=5;s.W4=s.Checksum(0,32);c.W10=1;c.W4=c.Checksum(0,32);
  foreach(int word in new[]{4,5,7,8,9,10,12}){
   int before=c.Word(word);c.SetWord(word,before^1);if(word!=4)c.W4=c.Checksum(0,32);
   // A one-unit source difference is tolerated; test outside that tolerance.
   if(word==12){c.W12=66666800;c.W4=c.Checksum(0,32);}
   Check(!client.FoliageMotionReady(menu,true));c.SetWord(word,before);c.W4=c.Checksum(0,32);
  }
  foreach(int phase in new[]{0,1,2,3,6,7,8,9}){s.W10=phase;s.W4=s.Checksum(0,32);Check(!client.FoliageMotionReady(menu,false));}
  s.W10=4;s.W4=s.Checksum(0,32);Check(!client.FoliageMotionReady(menu,true)&&client.FoliageMotionReady(menu,false));
  s.W10=5;s.W17=0;s.W4=s.Checksum(0,32);Check(!client.FoliageMotionReady(menu,false));
  s.W17=1;s.W11=1;s.W4=s.Checksum(0,32);Check(!client.FoliageMotionReady(menu,false));
  s.W11=0;s.W18=1;s.W4=s.Checksum(0,32);Check(!client.FoliageMotionReady(menu,false));
  s.W18=2;s.W4=s.Checksum(0,32);s.W4^=1;Check(!client.FoliageMotionReady(menu,false));s.W4^=1;
  foreach(int word in new[]{5,7,8,9,16}){int before=s.Word(word);s.SetWord(word,before+1000);s.W4=s.Checksum(0,32);Check(!client.FoliageMotionReady(menu,false));s.SetWord(word,before);s.W4=s.Checksum(0,32);}
  menu.Authority.W20=1;Check(!client.FoliageMotionReady(menu,false));menu.Authority.W20=2;
  menu.Authority.W19++;Check(!client.FoliageMotionReady(menu,false));menu.Authority.W19--;
  menu.Ready=false;Check(!client.FoliageMotionReady(menu,false));menu.Ready=true;
  client.Enabled=false;Check(!client.FoliageMotionReady(menu,false));client.Enabled=true;
  client.WorldGeneration++;Check(!client.FoliageMotionReady(menu,false));client.WorldGeneration--;
  Check(client.FoliageMotionReady(menu,true));
  Console.WriteLine("FSR foliage admission: active output, current world/request, scale, corruption and retirement gates pass");
 }
 static void TemporalQualityChecks(){
  UGameplayStatics.Slots.Clear();var menu=Menu();menu.Authority!.W20=2;
  var client=new ProviderRuntimeClient();client.Start();var s=Plan(1,1);
  UGameplayStatics.Slots["MCD2GraphicsProviderSrRuntime"]=s;
  Check(client.Poll(menu,"Hub",true,100000,100000000)&&client.TemporalShaderPreflight(menu));
  foreach(int word in new[]{4,5,7,8,9,18}){int old=s.Word(word);s.SetWord(word,old^1);if(word!=4)s.W4=s.Checksum(0,32);
   Check(!client.TemporalShaderPreflight(menu));s.SetWord(word,old);s.W4=s.Checksum(0,32);}
  s.W11=22;s.W4=s.Checksum(0,32);Check(!client.TemporalShaderPreflight(menu));s.W11=0;
  foreach(int phase in new[]{0,2,3,4,5,6,7,8,9}){s.W10=phase;s.W4=s.Checksum(0,32);Check(!client.TemporalShaderPreflight(menu));}
  s.W10=1;s.W4=s.Checksum(0,32);client.Context!.W10=0;client.Context.W4=client.Context.Checksum(0,32);Check(!client.TemporalShaderPreflight(menu));
  foreach(int original in new[]{0,1}){
   var lease=new TemporalAaQualityLeaseSave();
   Check(lease.Command(123,7,99,true,original,false,true)==-1&&lease.Owned==0); // Native never acquires.
   Check(lease.Command(123,7,99,false,original,true,true)==-1);
   Check(lease.Command(123,7,99,true,original,true,true)==2);lease.Observed(2,2);
   Check(lease.Owned==1&&lease.Original==original);
   Check(lease.Command(123,7,99,true,2,false,true)==-1&&lease.Owned==1); // World travel/retirement retain.
   var traveled=new TemporalAaQualityLeaseSave{SchemaVersion=1,Session=lease.Session,Original=lease.Original,Owned=lease.Owned,Revision=lease.Revision,Checksum=lease.Checksum};
   Check(traveled.Command(123,7,99,true,2,false,true)==-1&&traveled.Owned==1);
   Check(traveled.Command(123,8,100,true,2,false,false)==original);traveled.Observed(original,original);Check(traveled.Owned==0);
   Check(lease.Command(123,7,99,true,0,false,true)==-1&&lease.Owned==0&&lease.Blocked==1); // External edit wins.
   Check(lease.Command(123,7,99,true,0,true,true)==-1);
   Check(lease.Command(123,8,100,true,0,true,true)==2);lease.Observed(2,1);Check(lease.Owned==0&&lease.Blocked==1); // Failed command.
   var stale=new TemporalAaQualityLeaseSave{SchemaVersion=1,Session=123,Owned=1,Original=original};
   Check(stale.Command(124,7,99,true,2,false,false)==-1&&stale.Owned==0); // No cross-process restore.
  }
  var retry=new TemporalAaQualityLeaseSave{SchemaVersion=1,Session=123,Owned=1,Original=1};
  for(int n=0;n<3;n++){Check(retry.Command(123,7,99,true,2,false,false)==1);retry.Observed(1,2);}
  Check(retry.Command(123,7,99,true,2,false,false)==-1&&retry.Owned==1);
  Console.WriteLine("Temporal producer quality: preflight identity, travel, rollback, external edits and bounded restoration pass");
 }
}
