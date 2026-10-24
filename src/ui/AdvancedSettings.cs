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
 // Loader choice indices are not NGX preset numbers. Keep the old numeric save
 // key and wire format intact when migrating to the named selector.
 public static int PresetValue(int choice,int current){
  if(choice==0)return 0;if(choice==1)return 5;if(choice==2)return 6;
  if(choice==3)return 10;if(choice==4)return 11;if(choice==5)return 12;if(choice==6)return 13;
  return choice==7?current:-1;
 }
 public static int PreviousPresetValue(int choice,int current){
  if(choice==0)return 0;if(choice==1)return 11;if(choice==2)return 10;
  if(choice==3)return 13;if(choice==4)return 12;if(choice==5)return 5;if(choice==6)return 6;
  return choice==7?current:-1;
 }
 public static int PresetChoice(int value){
  if(value==0)return 0;if(value==5)return 1;if(value==6)return 2;
  if(value==10)return 3;if(value==11)return 4;if(value==12)return 5;if(value==13)return 6;return 7;
 }
 public static string PresetName(int value){
  if(value==0)return "Auto (quality-dependent)";
  if(value==11)return "K - Transformer";if(value==10)return "J - Transformer";
  if(value==13)return "M - Transformer 2nd Gen";if(value==12)return "L - Transformer 2nd Gen";
  if(value==5)return "E - CNN (Legacy)";if(value==6)return "F - CNN (Legacy)";
  return "Custom (manual override: "+value+")";
 }
 public string? Slot;
 public string SlotName(){return Slot==null||Slot==""?"MCD2GraphicsAdvanced":Slot;}
 public AdvancedStateSave? State;
 public AdvancedRequestSave? Pending;
 public int Hint;public bool Migrate;public bool Saved;public int Failure;
 int session;int sequence;int ticks;int retries;int sentHint;bool sentMigration;
 public void Start(){
  var seed=UGameplayStatics.LoadGameFromSlot(SlotName()+"State",0) as AdvancedStateSave;
  if(seed==null){seed=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<AdvancedStateSave>()) as AdvancedStateSave;if(seed!=null)UGameplayStatics.SaveGameToSlot(seed,SlotName()+"State",0);}
 }
 public void Select(int hint,bool migrate){
  if(hint<0 || hint>255)return;
  if(Hint==hint && Migrate==migrate && Failure==0)return;
  Hint=hint;Migrate=migrate;Saved=false;Failure=0;retries=0;
 }
 public bool Valid(AdvancedStateSave a){
  if(a.W0!=843334477 || a.W1!=3232321 || (a.W2!=1 && a.W2!=2) || a.W3!=128 || a.W4!=a.Checksum() || a.W5<1 ||
     a.W6<0 || a.W7<0 || a.W7>7 || a.W10<0 || a.W10>255 || a.W11<0 || a.W11>255 || a.W12<0 || a.W12>1 || a.W13<0 || a.W13>1023)return false;
  if(a.W2==1 && (a.W13>255 || a.W30!=0 || a.W31!=0))return false;
  if((a.W13&512)!=0 && (a.W13&256)==0)return false;
  if((a.W13&512)==0 && (a.W30!=0 || a.W31!=0))return false;
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
  var next=UGameplayStatics.LoadGameFromSlot(SlotName()+"State",0) as AdvancedStateSave;
  if(next==null || !Valid(next) || next.W5!=authority.W5){State=null;Saved=false;return;}
  State=next;
  if(session!=authority.W5){session=authority.W5;sequence=next.W6;Pending=null;Saved=false;Failure=0;retries=0;
   var old=UGameplayStatics.LoadGameFromSlot(SlotName()+"Request",0) as AdvancedRequestSave;
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
  if(!UGameplayStatics.SaveGameToSlot(r,SlotName()+"Request",0)){Failure=8;return;}
  Pending=r;sentHint=Hint;sentMigration=Migrate;ticks=0;
 }
 public string Version(int index){
  if(index<0 || index>4)return "Not reported";
  if(State==null)return "Waiting";
  if(index==4){
   if(State.W2<2)return "Not reported";
   if((State.W13&256)==0)return "Not loaded";
   if((State.W13&512)==0)return "Version unavailable";
   return ""+(State.W30&65535)+"."+((State.W30>>16)&65535)+"."+(State.W31&65535)+"."+((State.W31>>16)&65535);
  }
  if((State.W13&(1<<index))==0)return "Not loaded";
  if((State.W13&(1<<(index+4)))==0)return "Version unavailable";
  int first=14+index*4;return ""+State.Word(first)+"."+State.Word(first+1)+"."+State.Word(first+2)+"."+State.Word(first+3);
 }
 public string Status(){
  string text=Failure!=0?"Could not save model preset. Reset or retry.":State==null?"Waiting for settings connection.":!Saved?"Saving model preset...":State.W10!=State.W11?"Model preset saved. Restart to apply.":"Model preset saved.";
  return text+"\nSelected: "+PresetName(Hint);
 }
 public string Summary(){
  return Status()+"\n\nDLSS SR preset (active): Not reported\nDLSS FG preset (active): Not reported\n\nLoaded runtime versions\nNVIDIA DLSS Super Resolution: "+Version(0)+"\nNVIDIA DLSS Frame Generation: "+Version(4)+"\nAMD FSR Upscaling: "+Version(2)+"\nAMD FSR Frame Generation: "+Version(3)+"\nNVIDIA Streamline: "+Version(1);
 }
}

// Capability-filtered fixed-count choices. This never claims rendering activity.
public static class MfgModes {
 public static bool Any(int providers,int softwareMask,int mixedMask){
  return providers>=0 && providers<=3 && (providers!=0 || (softwareMask>0 && (softwareMask&~124)==0) || (mixedMask>0 && (mixedMask&~80)==0));
 }
 // UI intent only: custom startup leaves Streamline's maximum unknown.
 // Positive adapter support permits a baseline next-restart request, never activation.
 public static bool NativeRestartNeeded(int strategy,int sr,int fg,int multiplier,int providers,int startupOwner,int nativeMaximumGenerated){
  return (strategy==0 || strategy==1) && (sr==1 || sr==2) && fg==0 && multiplier==2 &&
   providers>0 && providers<=3 && (providers&1)!=0 && startupOwner==4 && nativeMaximumGenerated==0;
 }
 public static bool Selected(int strategy,int sr,int fg,int multiplier,int providers,int softwareMask,int mixedMask,int nativeMaximumGenerated,int startupOwner=0){
  if(strategy==2 || strategy==3)return Supported(strategy,sr,fg,multiplier,softwareMask,mixedMask,nativeMaximumGenerated);
  if(providers<0 || providers>3)return false;
  if(strategy==1)return (providers&1)!=0 && (Supported(strategy,sr,fg,multiplier,softwareMask,mixedMask,nativeMaximumGenerated) || NativeRestartNeeded(strategy,sr,fg,multiplier,providers,startupOwner,nativeMaximumGenerated));
  return strategy==0 && multiplier==2 && (fg==0?(providers&1)!=0:fg==1 && (providers&2)!=0);
 }
 public static bool Supported(int strategy,int sr,int fg,int multiplier,int softwareMask,int mixedMask,int nativeMaximumGenerated){
  if(multiplier<2 || multiplier>6 || (sr!=1 && sr!=2))return false;
  if(strategy==0 || strategy==1)return fg==0 && nativeMaximumGenerated>=multiplier-1 && nativeMaximumGenerated<=5;
  if(strategy==3)return fg==1 && softwareMask>=0 && (softwareMask&~124)==0 && (softwareMask&(1<<multiplier))!=0;
  return strategy==2 && fg==0 && (multiplier==4 || multiplier==6) && mixedMask>=0 && (mixedMask&~80)==0 && (mixedMask&(1<<multiplier))!=0;
 }
 public static int NextCount(int strategy,int sr,int fg,int current,int direction,int softwareMask,int mixedMask,int nativeMaximumGenerated){
  if(direction==0)return current;int count=current;
  if(count<2 || count>6)count=direction>0?6:2;
  for(int n=0;n<5;n++){count+=direction>0?1:-1;if(count<2)count=6;if(count>6)count=2;
   if(Supported(strategy,sr,fg,count,softwareMask,mixedMask,nativeMaximumGenerated))return count;}
  return current;
 }
}

// Generated-frame allocations. Multipliers include one rendered frame.
public static class FgAllocation {
 public static bool Valid(int dlss,int fsr){return dlss>=0 && fsr>=0 && dlss<=5 && fsr<=5 && dlss+fsr>=1 && dlss+fsr<=5;}
 public static int Strategy(int dlss,int fsr){return dlss+fsr==1?0:dlss==0?3:fsr==0?1:2;}
 public static int Provider(int dlss,int fsr){return dlss>0?0:1;}
 public static int Pack(int strategy,int dlss,int fsr){return strategy|(dlss<<8)|(fsr<<16);}
 public static int Dlss(int word,int provider,int total){int d=(word>>8)&255,f=(word>>16)&255;if(d+f!=0)return d;int strategy=word&255;return strategy==2?1:strategy==3 || provider!=0?0:total-1;}
 public static int Fsr(int word,int provider,int total){int d=(word>>8)&255,f=(word>>16)&255;if(d+f!=0)return f;return total-1-Dlss(word,provider,total);}
 public static bool WireValid(int schema,int word,int provider,int total){
  if((schema!=5 && schema!=6) || word<0 || (word&unchecked((int)0xff000000))!=0 || (word&255)>3)return false;
  int d=(word>>8)&255,f=(word>>16)&255;if(schema==5)return d==0 && f==0;
  if(d+f==0)return true;
  return Valid(d,f) && total==d+f+1 && provider==Provider(d,f) && (word&255)==Strategy(d,f);
 }
 static int Gcd(int a,int b){while(b!=0){int r=a%b;a=b;b=r;}return a;}
 public static int NativeMaximumNeeded(int dlss,int fsr){
  if(dlss==0)return 0;if(!Valid(dlss,fsr))return 6;int total=dlss+fsr+1,g=total;
  for(int j=1;j<=dlss;j++){int slot=(2*j*total+dlss+1)/(2*(dlss+1));g=Gcd(g,slot);}return total/g-1;
 }
 public static bool Supported(int dlss,int fsr,int sr,int providers,int softwareMask,int nativeMaximum,int mixedNativeMaximum,int startupOwner,int vendor){
  if(!Valid(dlss,fsr) || providers<0 || providers>3 || (softwareMask&~124)!=0 || nativeMaximum<0 || nativeMaximum>5 || mixedNativeMaximum<0 || mixedNativeMaximum>5)return false;
  if(dlss>0 && vendor!=0x10de)return false;
  int total=dlss+fsr+1;
  if(total==2)return dlss==1?(providers&1)!=0 && (sr==1 || sr==2) && (nativeMaximum>=1 || startupOwner==4):(providers&2)!=0 && sr>=0 && sr<=2;
  if(sr!=1 && sr!=2)return false;
  if(fsr==0)return (providers&1)!=0 && nativeMaximum>=dlss;
  if((softwareMask&(1<<total))==0)return false;
  return dlss==0 || mixedNativeMaximum>=NativeMaximumNeeded(dlss,fsr);
 }
}
