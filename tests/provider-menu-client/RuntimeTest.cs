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
  Check(ProviderRuntimeClient.CanRestoreNvidiaSource(7,2,123,1,7,123,3));
  Check(!ProviderRuntimeClient.CanRestoreNvidiaSource(7,2,123,1,7,124,3));
  Check(!ProviderRuntimeClient.CanRestoreNvidiaSource(7,2,123,1,8,123,3));
  Check(!ProviderRuntimeClient.CanRestoreNvidiaSource(7,2,123,1,7,123,1));
  Check(!ProviderRuntimeClient.CanRestoreNvidiaSource(7,0,123,1,7,123,3));
  Check(!ProviderRuntimeClient.CanRestoreNvidiaSource(7,2,0,1,7,123,3));
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
}
