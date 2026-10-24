using UE.Engine;
static class AdvancedTest {
 static void Need(bool value){if(!value)throw new Exception("Advanced settings assertion failed");}
 static AdvancedStateSave State(int session=123,int sequence=0,int status=0,int hint=12,int migrated=0){
  var s=new AdvancedStateSave{W0=0x3244434d,W1=0x00315241,W2=1,W3=128,W5=session,W6=sequence,W7=status,W10=hint,W11=12,W12=migrated};
  if(sequence!=0){s.W8=7;s.W9=unchecked((int)0xfedcba98);}
  s.W4=s.Checksum();UGameplayStatics.Slots["MCD2GraphicsAdvancedState"]=s;return s;
 }
 public static void Run(){
  int[] presets={0,5,6,10,11,12,13};
  int[] previous={0,11,10,13,12,5,6};
  for(int i=0;i<presets.Length;i++){
   Need(AdvancedMenuClient.PresetValue(i,99)==presets[i]);
   Need(AdvancedMenuClient.PresetChoice(presets[i])==i);
   Need(AdvancedMenuClient.PreviousPresetValue(i,99)==previous[i]);
  }
  Need(AdvancedMenuClient.PresetChoice(99)==7&&AdvancedMenuClient.PresetValue(7,99)==99);
  Need(AdvancedMenuClient.PresetValue(-1,11)==-1&&AdvancedMenuClient.PresetValue(8,11)==-1);
  Need(AdvancedMenuClient.PresetName(0)=="Auto (quality-dependent)");
  Need(AdvancedMenuClient.PresetName(11)=="K - Transformer");
  Need(AdvancedMenuClient.PresetName(99)=="Custom (manual override: 99)");
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
  Need(c.Summary().Contains("Selected: Auto (quality-dependent)")&&!c.Summary().Contains("hint"));
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
  Need(c.Version(4)=="Not reported");
  s.W13=16;s.W4=s.Checksum();Need(!c.Valid(s));
  s.W13=1;s.W14=0;s.W15=0;s.W16=0;s.W4=s.Checksum();Need(c.Valid(s));
  s.W2=2;s.W13=769;s.W30=310|(9<<16);s.W31=-1;s.W4=s.Checksum();Need(c.Valid(s));
  Need(c.Version(4)=="310.9.65535.65535");
  s.W13=513;s.W4=s.Checksum();Need(!c.Valid(s));
  s.W13=257;s.W4=s.Checksum();Need(!c.Valid(s));
  s.W30=0;s.W31=0;s.W4=s.Checksum();Need(c.Valid(s)&&c.Version(4)=="Version unavailable");
  s.W13=1;s.W4=s.Checksum();Need(c.Valid(s)&&c.Version(4)=="Not loaded");
  Need(c.Summary().Contains("DLSS FG preset (active): Not reported"));
  var travel=new AdvancedMenuClient();State(session:124,sequence:19,status:1);travel.Poll(core);Need(travel.Pending!=null&&travel.Pending.W6==20);
  var separate=new AdvancedMenuClient{Slot="MCD2GraphicsAdvancedFG",Hint=2};separate.Start();
  var fgState=State(session:124);UGameplayStatics.Slots["MCD2GraphicsAdvancedFGState"]=fgState;
  separate.Poll(core);Need(separate.Pending!=null&&separate.Pending.W9==2&&UGameplayStatics.Slots.ContainsKey("MCD2GraphicsAdvancedFGRequest"));
  Need(!ReferenceEquals(separate.Pending,UGameplayStatics.Slots["MCD2GraphicsAdvancedRequest"]));
  Console.WriteLine("Advanced menu: migration, reset, latest-edit ACK, stale session, retry, version validation and travel sequence checks pass");
 }
}
