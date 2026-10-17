using NeoRune;
using UE.CoreUObject;
using UE.Engine;

public class AdvancedRequestSave : USaveGame {
 public int W0,W1,W2,W3,W4,W5,W6,W7,W8,W9,W10,W11,W12,W13,W14,W15;
 public int Word(int index){
  if(index==0)return W0;
  if(index==1)return W1;
  if(index==2)return W2;
  if(index==3)return W3;
  if(index==4)return W4;
  if(index==5)return W5;
  if(index==6)return W6;
  if(index==7)return W7;
  if(index==8)return W8;
  if(index==9)return W9;
  if(index==10)return W10;
  if(index==11)return W11;
  if(index==12)return W12;
  if(index==13)return W13;
  if(index==14)return W14;
  if(index==15)return W15;
  return 0;
 }
 public void SetWord(int index,int value){
  if(index==0){W0=value;return;}
  if(index==1){W1=value;return;}
  if(index==2){W2=value;return;}
  if(index==3){W3=value;return;}
  if(index==4){W4=value;return;}
  if(index==5){W5=value;return;}
  if(index==6){W6=value;return;}
  if(index==7){W7=value;return;}
  if(index==8){W8=value;return;}
  if(index==9){W9=value;return;}
  if(index==10){W10=value;return;}
  if(index==11){W11=value;return;}
  if(index==12){W12=value;return;}
  if(index==13){W13=value;return;}
  if(index==14){W14=value;return;}
  if(index==15){W15=value;return;}
 }
 public int Checksum(){
  int crc=-1;
  for(int index=0;index<16;index++){
   int word=index==4?0:Word(index);
   for(int byteIndex=0;byteIndex<4;byteIndex++){
    crc=crc^(word&255);int nextWord=(word&2147483647)/256;if(word<0)nextWord+=8388608;word=nextWord;
    for(int bit=0;bit<8;bit++){int low=crc&1;int high=crc<0?1073741824:0;crc=((crc&2147483647)/2)+high;if(low!=0)crc=crc^-306674912;}
   }
  }
  return ~crc;
 }
}
public class AdvancedStateSave : USaveGame {
 public int W0,W1,W2,W3,W4,W5,W6,W7,W8,W9,W10,W11,W12,W13,W14,W15,W16,W17,W18,W19,W20,W21,W22,W23,W24,W25,W26,W27,W28,W29,W30,W31;
 public int Word(int index){
  if(index==0)return W0;
  if(index==1)return W1;
  if(index==2)return W2;
  if(index==3)return W3;
  if(index==4)return W4;
  if(index==5)return W5;
  if(index==6)return W6;
  if(index==7)return W7;
  if(index==8)return W8;
  if(index==9)return W9;
  if(index==10)return W10;
  if(index==11)return W11;
  if(index==12)return W12;
  if(index==13)return W13;
  if(index==14)return W14;
  if(index==15)return W15;
  if(index==16)return W16;
  if(index==17)return W17;
  if(index==18)return W18;
  if(index==19)return W19;
  if(index==20)return W20;
  if(index==21)return W21;
  if(index==22)return W22;
  if(index==23)return W23;
  if(index==24)return W24;
  if(index==25)return W25;
  if(index==26)return W26;
  if(index==27)return W27;
  if(index==28)return W28;
  if(index==29)return W29;
  if(index==30)return W30;
  if(index==31)return W31;
  return 0;
 }
 public int Checksum(){
  int crc=-1;
  for(int index=0;index<32;index++){
   int word=index==4?0:Word(index);
   for(int byteIndex=0;byteIndex<4;byteIndex++){
    crc=crc^(word&255);int nextWord=(word&2147483647)/256;if(word<0)nextWord+=8388608;word=nextWord;
    for(int bit=0;bit<8;bit++){int low=crc&1;int high=crc<0?1073741824:0;crc=((crc&2147483647)/2)+high;if(low!=0)crc=crc^-306674912;}
   }
  }
  return ~crc;
 }
}

public class AdvancedMenuClient : USaveGame {
 public AdvancedStateSave? State;
 public AdvancedRequestSave? Pending;
 public int Hint;public bool Migrate;public bool Saved;public int Failure;
 int session;int sequence;int ticks;int retries;int sentHint;bool sentMigration;
 public void Start(){
  var seed=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsAdvancedState",0) as AdvancedStateSave;
  if(seed==null){seed=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<AdvancedStateSave>()) as AdvancedStateSave;if(seed!=null)UGameplayStatics.SaveGameToSlot(seed,"MCD2GraphicsAdvancedState",0);}
 }
 public void Select(int hint,bool migrate){
  if(hint<0 || hint>255)return;
  if(Hint==hint && Migrate==migrate && Failure==0)return;
  Hint=hint;Migrate=migrate;Saved=false;Failure=0;retries=0;
 }
 public bool Valid(AdvancedStateSave a){
  if(a.W0!=843334477 || a.W1!=3232321 || a.W2!=1 || a.W3!=128 || a.W4!=a.Checksum() || a.W5<1 ||
     a.W6<0 || a.W7<0 || a.W7>7 || a.W10<0 || a.W10>255 || a.W11<0 || a.W11>255 || a.W12<0 || a.W12>1 || a.W13<0 || a.W13>255 || a.W30!=0 || a.W31!=0)return false;
  if(a.W6==0 && (a.W7!=0 || a.W8!=0 || a.W9!=0 || a.W12!=0))return false;
  if(a.W6!=0 && a.W8<1)return false;
  for(int i=0;i<4;i++){
   bool loaded=(a.W13&(1<<i))!=0,version=(a.W13&(1<<(i+4)))!=0;
   if(version && !loaded)return false;
   for(int n=0;n<4;n++){int value=a.Word(14+i*4+n);if(value<0 || value>65535 || (!version && value!=0))return false;}
  }
  return true;
 }
 public void Poll(ProviderMenuClient core){
  if(!core.Ready || core.Authority==null){State=null;Saved=false;return;}
  var authority=core.Authority;
  var next=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsAdvancedState",0) as AdvancedStateSave;
  if(next==null || !Valid(next) || next.W5!=authority.W5){State=null;Saved=false;return;}
  State=next;
  if(session!=authority.W5){session=authority.W5;sequence=next.W6;Pending=null;Saved=false;Failure=0;retries=0;
   var old=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsAdvancedRequest",0) as AdvancedRequestSave;
   if(old!=null && old.W0==843334477 && old.W1==3232065 && old.W2==1 && old.W3==64 && old.W4==old.Checksum() && old.W5==session && old.W6>sequence)sequence=old.W6;
  }
  if(Pending!=null){
   if(next.W6==Pending.W6 && next.W8==Pending.W7 && next.W9==Pending.W8){
    if(next.W7==1){
     bool current=sentHint==Hint && sentMigration==Migrate;
     if(current){if(sentMigration)Hint=next.W10;Migrate=false;Saved=true;Failure=0;}
     Pending=null;retries=0;
    }else if(next.W7==2 || next.W7==5){Pending=null;retries++;if(retries>8)Failure=2;}
    else if(next.W7!=0){Failure=next.W7;Pending=null;}
   }else{
    ticks++;if(ticks>=20){Pending=null;retries++;if(retries>3)Failure=8;}return;
   }
  }
  if(Saved || Failure!=0 || Pending!=null)return;
  if(sequence>=2147483646){Failure=8;return;}
  var r=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<AdvancedRequestSave>()) as AdvancedRequestSave;
  if(r==null){Failure=8;return;}
  r.W0=843334477;r.W1=3232065;r.W2=1;r.W3=64;r.W5=session;r.W6=++sequence;
  r.W7=authority.W19;r.W8=authority.W16;r.W9=Hint;r.W10=Migrate?1:0;r.W4=r.Checksum();
  if(!UGameplayStatics.SaveGameToSlot(r,"MCD2GraphicsAdvancedRequest",0)){Failure=8;return;}
  Pending=r;sentHint=Hint;sentMigration=Migrate;ticks=0;
 }
 public string Version(int index){
  if(State==null)return "Waiting";
  if((State.W13&(1<<index))==0)return "Not loaded";
  if((State.W13&(1<<(index+4)))==0)return "Version unavailable";
  int first=14+index*4;return ""+State.Word(first)+"."+State.Word(first+1)+"."+State.Word(first+2)+"."+State.Word(first+3);
 }
 public string Summary(){
  string text=Failure!=0?"Could not save model hint. Reset or retry.":State==null?"Waiting for settings connection.":!Saved?"Saving model hint...":State.W10!=State.W11?"Model hint saved. Restart to apply.":"Model hint saved.";
  return text+"\nRequested hint: "+Hint+" (0 = runtime default)\nDLSS SR: "+Version(0)+"\nStreamline: "+Version(1)+"\nFSR SR: "+Version(2)+"\nFSR FG: "+Version(3);
 }
}
