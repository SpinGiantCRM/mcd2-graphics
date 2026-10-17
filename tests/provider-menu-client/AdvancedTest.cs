using UE.Engine;
static class AdvancedTest {
 static void Need(bool value){if(!value)throw new Exception("Advanced settings assertion failed");}
 static AdvancedStateSave State(int session=123,int sequence=0,int status=0,int hint=12,int migrated=0){
  var s=new AdvancedStateSave{W0=0x3244434d,W1=0x00315241,W2=1,W3=128,W5=session,W6=sequence,W7=status,W10=hint,W11=12,W12=migrated};
  if(sequence!=0){s.W8=7;s.W9=unchecked((int)0xfedcba98);}
  s.W4=s.Checksum();UGameplayStatics.Slots["MCD2GraphicsAdvancedState"]=s;return s;
 }
 public static void Run(){
  UGameplayStatics.Slots.Clear();UGameplayStatics.SaveCount=0;UGameplayStatics.FailSave=false;
  var core=new ProviderMenuClient{Ready=true,Authority=new ProviderAuthoritySave{W5=123,W19=7,W16=unchecked((int)0xfedcba98)}};
  var c=new AdvancedMenuClient{Hint=0,Migrate=true};c.Start();Need(UGameplayStatics.SaveCount==1);
  c.Poll(core);Need(c.Pending==null&&!c.Saved); // Default seed is not a native ACK.
  var s=State();Need(c.Valid(s));s.W4^=1;Need(!c.Valid(s));State();
  c.Poll(core);Need(c.Pending!=null&&c.Pending.W6==1&&c.Pending.W10==1&&c.Pending.W9==0);
  Need(c.Pending!.W8==core.Authority.W16&&c.Pending.W4==c.Pending.Checksum());
  int writes=UGameplayStatics.SaveCount;c.Poll(core);Need(UGameplayStatics.SaveCount==writes);
  State(sequence:1,status:1,hint:19,migrated:1);c.Poll(core);Need(c.Saved&&!c.Migrate&&c.Hint==19&&c.Pending==null);
  c.Poll(core);Need(UGameplayStatics.SaveCount==writes);
  c.Select(0,false);c.Poll(core);Need(c.Pending!=null&&c.Pending.W6==2&&c.Pending.W9==0&&c.Pending.W10==0);
  State(sequence:2,status:1,hint:0);c.Poll(core);Need(c.Saved&&c.Hint==0);
  c.Select(256,false);Need(c.Hint==0&&c.Saved);
  c.Select(30,false);c.Poll(core);Need(c.Pending!=null&&c.Pending.W6==3);
  c.Select(31,false);State(sequence:3,status:1,hint:30);c.Poll(core);Need(!c.Saved&&c.Pending!=null&&c.Pending.W6==4&&c.Pending.W9==31);
  State(sequence:4,status:2,hint:30);c.Poll(core);Need(c.Pending!=null&&c.Pending.W6==5);
  State(sequence:5,status:4,hint:30);c.Poll(core);Need(c.Pending==null&&c.Failure==4);
  c.Select(0,false);c.Poll(core);Need(c.Pending!=null&&c.Pending.W6==6);
  State(session:124);c.Poll(core);Need(c.State==null&&!c.Saved);
  core.Authority.W5=124;c.Poll(core);Need(c.Pending!=null&&c.Pending.W5==124&&c.Pending.W6==1);
  s=State(session:124,sequence:1,status:1,hint:0);s.W13=17;s.W14=310;s.W15=9;s.W16=1;s.W4=s.Checksum();
  c.Poll(core);Need(c.Version(0)=="310.9.1.0"&&c.Version(1)=="Not loaded");
  s.W13=16;s.W4=s.Checksum();Need(!c.Valid(s));
  s.W13=1;s.W14=0;s.W15=0;s.W16=0;s.W4=s.Checksum();Need(c.Valid(s));
  var travel=new AdvancedMenuClient();State(session:124,sequence:19,status:1);travel.Poll(core);Need(travel.Pending!=null&&travel.Pending.W6==20);
  Console.WriteLine("Advanced menu: migration, reset, latest-edit ACK, stale session, retry, version validation and travel sequence checks pass");
 }
}
