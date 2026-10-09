using NeoRune;
using UE.Engine;

// Game-thread request client. Only the native settings worker writes the
// committed authority. No setting, receipt or menu selection proves activation.
public class ProviderMenuClient : USaveGame {
 public ProviderAuthoritySave? Authority;
 public ProviderRequestSave? Edits;
 public ProviderRequestSave? Pending;
 public int Dirty;
 public int PendingMask;
 public int RetryCount;
 public int WaitTicks;
 public int ReceiptStatus;
 public int Failure;
 public bool Ready;
 public int Bit(int word){int bit=1;for(int n=0;n<word;n++)bit*=2;return bit;}
 public bool ScaleValid(int n){return n>=100 && n<=10000 && n%100==0;}
 public bool RecordValid(ProviderAuthoritySave a){
  if(a.W12!=843334477 || a.W13!=3492167 || a.W14!=1 || a.W15!=128 ||
     a.W16!=a.Checksum(12,32) || a.W18!=5 || a.W19<1 || a.W20<0 || a.W20>3)return false;
  for(int first=21;first<=27;first+=3)if(a.Word(first)<0 || a.Word(first)>5 ||
     !ScaleValid(a.Word(first+1)) || !ScaleValid(a.Word(first+2)))return false;
  if(a.W30<0 || a.W30>1 || a.W31<0 || a.W31>1 || a.W32<0 || a.W32>2 ||
     a.W33<0 || a.W33>2 || a.W34<2 || a.W34>16 || (a.W33==0 && a.W34!=2) ||
     a.W35<0 || a.W35>3 || a.W36<0 || a.W36>2 || a.W37<0 || a.W37>1 ||
     a.W38<100 || a.W38>10000 || a.W38%10!=0 || a.W39<48 || a.W39>500 ||
     a.W40<48 || a.W40>500 || a.W17!=(a.W31==0?0:a.W32+1))return false;
  bool empty=a.W41==0 && a.W42==0 && a.W43==0;
  if(!empty && (a.W41<1 || a.W42<1 || a.W43<1 || a.W41>a.W19 || a.W42>a.W19 || a.W43>a.W19))return false;
  return true;
 }
 public bool AuthorityValid(ProviderAuthoritySave a){
  if(a.W0!=843334477 || a.W1!=3232577 || a.W2!=1 || a.W3!=176 || a.W5<1 ||
     a.W4!=a.Checksum(0,44) || a.W8!=0 || !RecordValid(a))return false;
  if(a.W6==0)return a.W7==0 && a.W9==0 && a.W10==0 && a.W11==2;
  if(a.W6<1 || a.W7<0 || a.W7>12 || a.W9<0 || (a.W9==0 && a.W10!=0))return false;
  int store=2;
  if(a.W7==9 || a.W7==10)store=0;
  if(a.W7==11)store=6;
  if(a.W7==4)store=1;
  if(a.W7==5 || a.W7==6 || a.W7==12)store=5;
  if(a.W7==7)store=4;
  if(a.W7==8)return a.W11==1 || a.W11==2 || a.W11==3;
  return a.W11==store && (a.W7<9 || a.W7>11 || a.W9>0);
 }
 public bool RequestValid(ProviderRequestSave r){
  if(r.W0!=843334477 || r.W1!=3232082 || r.W2!=1 || r.W3!=168 || r.W5<1 ||
     r.W6<1 || r.W7<1 || r.W7>=2147483647 || r.W9!=0 || r.W4!=r.Checksum(0,42) ||
     r.W17!=r.W7+1 || r.W14!=r.Checksum(10,32))return false;
  var a=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderAuthoritySave>()) as ProviderAuthoritySave;
  if(a==null)return false;
  // Reuse the identical inner-record validation without claiming an outer ACK.
  a.W12=r.W10;a.W13=r.W11;a.W14=r.W12;a.W15=r.W13;a.W16=r.W14;a.W17=r.W15;
  a.W18=r.W16;a.W19=r.W17;a.W20=r.W18;a.W21=r.W19;a.W22=r.W20;a.W23=r.W21;
  a.W24=r.W22;a.W25=r.W23;a.W26=r.W24;a.W27=r.W25;a.W28=r.W26;a.W29=r.W27;
  a.W30=r.W28;a.W31=r.W29;a.W32=r.W30;a.W33=r.W31;a.W34=r.W32;a.W35=r.W33;
  a.W36=r.W34;a.W37=r.W35;a.W38=r.W36;a.W39=r.W37;a.W40=r.W38;a.W41=r.W39;a.W42=r.W40;a.W43=r.W41;
  return RecordValid(a);
 }
 public void Start(){
  var seed=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsProviderAuthority",0) as ProviderAuthoritySave;
  if(seed==null){seed=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderAuthoritySave>()) as ProviderAuthoritySave;
   if(seed!=null)UGameplayStatics.SaveGameToSlot(seed,"MCD2GraphicsProviderAuthority",0);}
 }
 public int Value(int word){
  if(word<8 || word>28)return 0;
  if(Edits!=null && (Dirty&Bit(word))!=0)return Edits.Word(10+word);
  if(Pending!=null && (PendingMask&Bit(word))!=0)return Pending.Word(10+word);
  return Authority==null?0:Authority.Word(12+word);
 }
 bool Edit(int word,int value){
  if(!Ready || word<8 || word>28)return false;
  if(Edits==null)Edits=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderRequestSave>()) as ProviderRequestSave;
  if(Edits==null)return false;
  Edits.SetWord(10+word,value);Dirty=Dirty|Bit(word);RetryCount=0;Failure=0;return true;
 }
 public bool SelectSr(int provider){if(provider<0 || provider>3)return false;return Edit(8,provider);}
 public bool SelectMenuSr(int provider){
  if(provider<0 || provider>2 || !Ready)return false;
  // AMD FG accepts both implemented SR guide producers; Native has no independent guide path yet.
  // Preserve the independent stored provider/strategy, but switch FG Off in
  // the same transaction before choosing Native or FSR.
  if((provider==0 || (provider==2 && Value(20)!=1)) && Value(19)!=0 && !Edit(19,0))return false;
  return SelectSr(provider);
 }
 public bool SelectAntiLag2(bool enabled){
  // FG coexistence needs its swapchain handshake; do not save an unsupported
  // pairing or reinterpret NVIDIA Boost as an AMD latency mode.
  return Ready && Value(19)==0 && SelectLatency(2,enabled?1:0);
 }
 public int PreferenceWord(int provider){return provider>=1 && provider<=3?9+(provider-1)*3:-1;}
 public bool SelectQuality(int provider,int quality){
  int word=PreferenceWord(provider);if(word<0 || quality<0 || quality>5 || !Ready)return false;
  if(quality==5 && !Edit(word+1,Value(word+2)))return false;
  return Edit(word,quality);
 }
 public bool SelectCustomScale(int provider,int scale){
  int word=PreferenceWord(provider);if(word<0 || !ScaleValid(scale) || !Ready)return false;
  // Ratios for named modes come from the selected SDK. A manual percentage
  // stays Custom until that provider's validated dimension query identifies it.
  if(scale==10000)return Edit(word,0);
  if(provider==1){int named=scale==6700?1:scale==5800?2:scale==5000?3:scale==3300?4:5;
   if(named!=5)return Edit(word,named);}
  return Edit(word+1,scale) && Edit(word+2,scale) && Edit(word,5);
 }
 public bool SelectFg(int provider,int strategy,int multiplier,bool enabled){
  if(provider<0 || provider>2 || strategy<0 || strategy>2 || multiplier<2 || multiplier>16 ||
     (strategy==0 && multiplier!=2) || !Ready)return false;
  return Edit(20,provider) && Edit(21,strategy) && Edit(22,multiplier) && Edit(19,enabled?1:0);
 }
 public bool SelectLatency(int provider,int mode){
  if(provider<0 || provider>3 || mode<0 || mode>2 || !Ready)return false;
  return Edit(23,provider) && Edit(24,mode);
 }
 public bool SelectHdr(bool enabled,int peak,int paper,int ui){
  if(peak<100 || peak>10000 || peak%10!=0 || paper<48 || paper>500 || ui<48 || ui>500 || !Ready)return false;
  return Edit(25,enabled?1:0) && Edit(26,peak) && Edit(27,paper) && Edit(28,ui);
 }
 public bool ResetDefaults(){
  if(!Ready || !Edit(8,0))return false;
  for(int word=9;word<=15;word+=3)if(!Edit(word,1) || !Edit(word+1,6700) || !Edit(word+2,7700))return false;
  return Edit(18,1) && SelectFg(0,0,2,false) && SelectLatency(0,1) && SelectHdr(false,1000,203,203);
 }
 public void RequeuePending(){
  if(Pending==null)return;
  for(int word=8;word<=28;word++)if((PendingMask&Bit(word))!=0 && (Dirty&Bit(word))==0){
   int retries=RetryCount;Edit(word,Pending.Word(10+word));RetryCount=retries;
  }
 }
 public void Poll(){
  var a=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsProviderAuthority",0) as ProviderAuthoritySave;
  Ready=a!=null && AuthorityValid(a);
  if(!Ready || a==null){Failure=1;return;}
  Authority=a;
  // Adopt an outstanding request after actor travel instead of overwriting a
  // sequence already sent. Its changed-field mask is not recoverable reliably.
  if(Pending==null){
   var old=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsProviderRequest",0) as ProviderRequestSave;
   if(old!=null && RequestValid(old) && old.W5==a.W5 && old.W6>a.W6){Pending=old;PendingMask=0;WaitTicks=0;}
  }
  if(Pending!=null){
   if(Pending.W5!=a.W5){RequeuePending();Pending=null;PendingMask=0;WaitTicks=0;}
   else if(a.W6==Pending.W6 && a.W7!=7){
    ReceiptStatus=a.W7;
    bool accepted=(a.W7==9 || a.W7==10 || a.W7==11) && a.W9==a.W19 && a.W10==a.W16;
    if(!accepted){RequeuePending();RetryCount++;Failure=2;}
    else {Failure=a.W7==11?3:0;RetryCount=0;}
    Pending=null;PendingMask=0;WaitTicks=0;
   }else if(a.W6>Pending.W6){RequeuePending();RetryCount++;Failure=2;Pending=null;PendingMask=0;WaitTicks=0;}
   else {WaitTicks++;if(WaitTicks>=120)Failure=4;return;}
  }
  if(Dirty==0 || RetryCount>=3)return;
  if(a.W19>=2147483647 || a.W6>=2147483647){Failure=5;return;}
  var r=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderRequestSave>()) as ProviderRequestSave;
  if(r==null){Failure=6;return;}
  r.W0=843334477;r.W1=3232082;r.W2=1;r.W3=168;r.W5=a.W5;r.W6=a.W6+1;r.W7=a.W19;r.W8=a.W16;
  for(int word=0;word<32;word++)r.SetWord(10+word,a.Word(12+word));
  for(int word=8;word<=28;word++)if((Dirty&Bit(word))!=0 && Edits!=null)r.SetWord(10+word,Edits.Word(10+word));
  r.W15=r.W29==0?0:r.W30+1;r.W17=a.W19+1;r.W14=r.Checksum(10,32);r.W4=r.Checksum(0,42);
  if(!RequestValid(r)){Failure=7;return;}
  if(!UGameplayStatics.SaveGameToSlot(r,"MCD2GraphicsProviderRequest",0)){Failure=6;return;}
  Pending=r;PendingMask=Dirty;Dirty=0;WaitTicks=0;
 }
 public string Status(){
  if(!Ready)return "Settings unavailable.";
  if(Failure==3)return "Settings saved; storage confirmation unavailable.";
  if(Failure!=0)return "Settings could not be confirmed.";
  if(Pending!=null || Dirty!=0)return "Saving settings...";
  return "";
 }
}
