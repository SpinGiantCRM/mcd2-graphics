using UE.Engine;
static class ControlsTest {
 public static int[]? Fixture;
 static void Check(bool ok,string name){if(!ok)throw new Exception(name);}
 static ProviderAuthoritySave Authority(){
  var a=new ProviderAuthoritySave{W0=0x3244434d,W1=0x00315341,W2=1,W3=176,W5=321,W11=2,
   W12=0x3244434d,W13=0x00354947,W14=1,W15=128,W18=5,W19=7,W21=1,W22=6700,W23=7700,
   W24=1,W25=6700,W26=7700,W27=1,W28=6700,W29=7700,W30=1,W34=2,W36=1,W38=1000,W39=203,W40=203};
  Seal(a);return a;
 }
 static void Seal(ProviderAuthoritySave a){a.W16=a.Checksum(12,32);a.W4=a.Checksum(0,44);}
 static ProviderMenuClient Client(ProviderAuthoritySave a){
  UGameplayStatics.Slots.Clear();UGameplayStatics.SaveCount=0;UGameplayStatics.FailSave=false;
  UGameplayStatics.Slots["MCD2GraphicsProviderAuthority"]=a;
  var c=new ProviderMenuClient();c.Poll();Check(c.Ready,"initial authority");return c;
 }
 static ProviderRequestSave Sent()=> (ProviderRequestSave)UGameplayStatics.Slots["MCD2GraphicsProviderRequest"];
 static void Ack(ProviderAuthoritySave a,ProviderRequestSave r,int status=9){
  if(status==9 || status==11){
   for(int word=0;word<32;word++)typeof(ProviderAuthoritySave).GetField("W"+(12+word))!.SetValue(a,r.Word(10+word));
  }
  a.W6=r.W6;a.W7=status;a.W11=status==11?6:status==5?5:0;a.W9=a.W19;a.W10=a.W16;Seal(a);
 }
 public static void Run(){
  var a=Authority();var c=Client(a);
  Check(c.SelectSr(2)&&c.SelectQuality(2,2)&&c.SelectCustomScale(2,7100),"AMD choices accepted");
  Check(c.SelectLatency(2,1)&&c.SelectHdr(true,420,203,167),"AMD/HDR edits");
  Check(c.SelectFg(0,0,2,true),"independent NVIDIA FG");c.Poll();var r=Sent();
  Fixture=Enumerable.Range(0,42).Select(r.Word).ToArray();
  Check(c.RequestValid(r)&&r.W18==2&&r.W22==5&&r.W23==7100&&r.W24==7100,"AMD preference serialized");
  Check(r.W19==1&&r.W20==6700&&r.W21==7700,"NVIDIA preferences preserved");
  Check(r.W30==0&&r.W29==1&&r.W15==1&&r.W33==2&&r.W34==1,"independent FG and latency");
  Check(UGameplayStatics.SaveCount==1&&c.Status()=="Saving settings...","single complete transaction");
  c.SelectSr(1);c.SelectQuality(1,0);c.Poll();Check(UGameplayStatics.SaveCount==1,"pending immutable");
  Ack(a,r);c.Poll();var r2=Sent();Check(r2.W6==2&&r2.W18==1&&r2.W19==0,"queued edits after ACK");
  Check(r2.W22==5&&r2.W23==7100&&r2.W29==1&&r2.W30==0,"per-provider preference and FG retained");
  Ack(a,r2);c.Poll();Check(c.Pending==null&&c.Status()=="","receipt is saved, never Active");
  Check(!c.SelectSr(4)&&!c.SelectQuality(0,1)&&!c.SelectCustomScale(2,17)&&
   !c.SelectFg(1,0,3,true)&&!c.SelectHdr(true,421,203,203),"invalid setters atomic");
  Check(c.Dirty==0,"invalid setters no dirty fields");

  a=Authority();c=Client(a);Check(c.SelectFg(0,0,2,true)&&c.SelectMenuSr(2),"menu FSR selection");c.Poll();r=Sent();
  Check(r.W18==2&&r.W29==0&&r.W30==0,"unqualified FSR/NVIDIA FG pairing disabled atomically");
  a=Authority();c=Client(a);Check(c.SelectFg(0,0,2,true)&&c.SelectMenuSr(1),"menu NVIDIA selection");c.Poll();r=Sent();
  Check(r.W18==1&&r.W29==1,"NVIDIA FG preference preserved with NVIDIA SR");
  a=Authority();c=Client(a);Check(!c.SelectMenuSr(3)&&c.Dirty==0,"unimplemented menu provider refused atomically");
  a=Authority();c=Client(a);c.SelectCustomScale(1,6700);c.Poll();r=Sent();Check(r.W19==1,"NVIDIA 67 percent retains Quality alias");
  a=Authority();c=Client(a);c.SelectCustomScale(2,6700);c.Poll();r=Sent();Check(r.W22==5,"AMD 67 percent stays Custom until exact SDK dimensions identify it");
  a=Authority();c=Client(a);Check(c.SelectAntiLag2(true),"Anti-Lag On request");c.Poll();r=Sent();Check(r.W33==2&&r.W34==1,"AMD latency provider and mode committed together");
  a=Authority();c=Client(a);Check(c.SelectAntiLag2(false),"Anti-Lag Off request");c.Poll();r=Sent();Check(r.W33==2&&r.W34==0,"AMD Off is independent of Reflex");
  a=Authority();c=Client(a);c.SelectFg(0,0,2,true);int before=c.Dirty;Check(!c.SelectAntiLag2(true)&&c.Dirty==before,"AMD FG pairing not admitted without owned-swapchain handshake");
  a=Authority();c=Client(a);c.SelectCustomScale(2,7000);c.Poll();r=Sent();
  c.SelectCustomScale(2,7200);a.W39=220;a.W19++;Seal(a);Ack(a,r,5);c.Poll();r2=Sent();
  Check(r2.W23==7200&&r2.W24==7200&&r2.W37==220&&r2.W7==8,"conflict rebases newest edits only");
  Check(r2.W6==2&&c.RequestValid(r2),"new immutable sequence on conflict");
  Ack(a,r2);c.Poll();Check(c.Dirty==0&&c.Pending==null,"conflict completion");
  c.SelectQuality(2,1);c.Poll();r=Sent();Ack(a,r,11);c.Poll();
  Check(c.Pending==null&&c.Failure==3&&c.Status().Contains("storage confirmation"),"durability honest");

  a=Authority();c=Client(a);c.SelectSr(2);c.Poll();r=Sent();
  a.W5=322;a.W6=0;Seal(a);c.Poll();r2=Sent();
  Check(r2.W5==322&&r2.W6==1&&r2.W18==2,"new session requeues known fields");
  var travelled=new ProviderMenuClient();travelled.Poll();
  Check(travelled.Pending==r2&&UGameplayStatics.SaveCount==2,"travel adopts pending unchanged");
  for(int tick=0;tick<120;tick++)travelled.Poll();
  Check(travelled.Failure==4&&UGameplayStatics.SaveCount==2,"lost ACK no unsafe sequence rewrite");
  Ack(a,r2);travelled.Poll();Check(travelled.Pending==null,"late receipt resolves wait");

  a=Authority();c=Client(a);c.SelectSr(2);UGameplayStatics.FailSave=true;c.Poll();
  Check(c.Pending==null&&c.Dirty!=0&&c.Failure==6,"save failure retains edits");
  UGameplayStatics.FailSave=false;c.Poll();Check(c.Pending!=null,"save retry exact intent");
  var saved=r=Sent();a.W20=9;Seal(a);c.Poll();Check(!c.Ready&&Sent()==saved,"valid CRC invalid enums refused");
  a.W20=0;Seal(a);a.W4^=1;c.Poll();Check(!c.Ready,"corrupt outer refused");
  a=Authority();a.W17=1;Seal(a);Check(!c.AuthorityValid(a),"inconsistent presentation owner refused");
  a=Authority();a.W6=1;a.W7=9;a.W11=0;Seal(a);Check(!c.AuthorityValid(a),"commit without stamp refused");

  a=Authority();c=Client(a);c.SelectSr(2);
  for(int attempt=0;attempt<3;attempt++){c.Poll();r=Sent();Ack(a,r,5);c.Poll();}
  Check(c.RetryCount==3&&c.Pending==null&&c.Dirty!=0,"bounded conflict retries");
  c.SelectSr(1);c.Poll();Check(Sent().W18==1&&c.Pending!=null,"new user edit can retry");
  a=Authority();c=Client(a);c.SelectSr(2);c.SelectCustomScale(2,8100);
  c.SelectLatency(2,1);c.SelectFg(1,0,2,true);c.SelectHdr(true,420,203,167);
  Check(c.ResetDefaults(),"reset accepted");c.Poll();r=Sent();
  Check(c.RequestValid(r)&&r.W18==0&&r.W29==0&&r.W30==0&&r.W15==0&&r.W33==0&&r.W34==1&&
   r.W35==0&&r.W36==1000&&r.W37==203&&r.W38==203,"complete reset transaction");
  for(int word=19;word<=25;word+=3)Check(r.Word(word)==1&&r.Word(word+1)==6700&&r.Word(word+2)==7700,"reset every provider preference");
  Console.WriteLine("Provider controls: independent SR/FG/latency/HDR, coalescing, conflict rebase, travel, receipts, bounds and save-failure checks pass");
 }
}
