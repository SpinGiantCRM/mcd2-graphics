using NeoRune;
using UE.CoreUObject;
using UE.Engine;

public class ProviderRequestSave : USaveGame {
 public int W0,W1,W2,W3,W4,W5,W6,W7,W8,W9,W10,W11,W12,W13,W14,W15,W16,W17,W18,W19,W20,W21,W22,W23,W24,W25,W26,W27,W28,W29,W30,W31,W32,W33,W34,W35,W36,W37,W38,W39,W40,W41;
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
  if(index==32)return W32;
  if(index==33)return W33;
  if(index==34)return W34;
  if(index==35)return W35;
  if(index==36)return W36;
  if(index==37)return W37;
  if(index==38)return W38;
  if(index==39)return W39;
  if(index==40)return W40;
  if(index==41)return W41;
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
  if(index==16){W16=value;return;}
  if(index==17){W17=value;return;}
  if(index==18){W18=value;return;}
  if(index==19){W19=value;return;}
  if(index==20){W20=value;return;}
  if(index==21){W21=value;return;}
  if(index==22){W22=value;return;}
  if(index==23){W23=value;return;}
  if(index==24){W24=value;return;}
  if(index==25){W25=value;return;}
  if(index==26){W26=value;return;}
  if(index==27){W27=value;return;}
  if(index==28){W28=value;return;}
  if(index==29){W29=value;return;}
  if(index==30){W30=value;return;}
  if(index==31){W31=value;return;}
  if(index==32){W32=value;return;}
  if(index==33){W33=value;return;}
  if(index==34){W34=value;return;}
  if(index==35){W35=value;return;}
  if(index==36){W36=value;return;}
  if(index==37){W37=value;return;}
  if(index==38){W38=value;return;}
  if(index==39){W39=value;return;}
  if(index==40){W40=value;return;}
  if(index==41){W41=value;return;}
 }
 public int Checksum(int first,int count){
  int crc=-1;
  for(int index=0;index<count;index++){
   int word=index==4?0:Word(first+index);
   for(int byteIndex=0;byteIndex<4;byteIndex++){
    crc=crc^(word&255);
    int nextWord=(word&2147483647)/256;if(word<0)nextWord+=8388608;word=nextWord;
    for(int bit=0;bit<8;bit++){int low=crc&1;int high=crc<0?1073741824:0;crc=((crc&2147483647)/2)+high;if(low!=0)crc=crc^-306674912;}
   }
  }
  return ~crc;
 }
}
public class ProviderAuthoritySave : USaveGame {
 public int W0,W1,W2,W3,W4,W5,W6,W7,W8,W9,W10,W11,W12,W13,W14,W15,W16,W17,W18,W19,W20,W21,W22,W23,W24,W25,W26,W27,W28,W29,W30,W31,W32,W33,W34,W35,W36,W37,W38,W39,W40,W41,W42,W43;
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
  if(index==32)return W32;
  if(index==33)return W33;
  if(index==34)return W34;
  if(index==35)return W35;
  if(index==36)return W36;
  if(index==37)return W37;
  if(index==38)return W38;
  if(index==39)return W39;
  if(index==40)return W40;
  if(index==41)return W41;
  if(index==42)return W42;
  if(index==43)return W43;
  return 0;
 }
 public int Checksum(int first,int count){
  int crc=-1;
  for(int index=0;index<count;index++){
   int word=index==4?0:Word(first+index);
   for(int byteIndex=0;byteIndex<4;byteIndex++){
    crc=crc^(word&255);
    int nextWord=(word&2147483647)/256;if(word<0)nextWord+=8388608;word=nextWord;
    for(int bit=0;bit<8;bit++){int low=crc&1;int high=crc<0?1073741824:0;crc=((crc&2147483647)/2)+high;if(low!=0)crc=crc^-306674912;}
   }
  }
  return ~crc;
 }
}
// Temporary qualification client: sends one preference-identical request per
// native session. It never changes user preferences or treats persistence as activation.
public class ProviderTransportProbe : USaveGame {
 public int SentSession;
 public int SentSequence;
 public int LoggedSequence;
 public void StartTransport(){
  var seed=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsProviderAuthority",0) as ProviderAuthoritySave;
  if(seed==null){seed=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderAuthoritySave>()) as ProviderAuthoritySave;
   if(seed!=null)UGameplayStatics.SaveGameToSlot(seed,"MCD2GraphicsProviderAuthority",0);}
 }
 public void PollTransport(){
  var a=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsProviderAuthority",0) as ProviderAuthoritySave;
  if(a==null || a.W0!=843334477 || a.W1!=3232577 || a.W2!=1 || a.W3!=176 || a.W5<1 ||
     a.W4!=a.Checksum(0,44) || a.W8!=0 || a.W12!=843334477 || a.W13!=3492167 ||
     a.W14!=1 || a.W15!=128 || a.W16!=a.Checksum(12,32) || a.W19<1)return;
  if(SentSession==a.W5){
   if(a.W6==SentSequence && LoggedSequence!=SentSequence){LoggedSequence=SentSequence;
    Log.Write("PROVIDER TRANSPORT RECEIPT session="+a.W5+" sequence="+a.W6+" status="+a.W7+" revision="+a.W19);}
   return;
  }
  if(a.W19>=2147483647 || a.W6>=2147483647)return;
  var r=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderRequestSave>()) as ProviderRequestSave;
  if(r==null)return;
  r.W0=843334477;r.W1=3232082;r.W2=1;r.W3=168;r.W5=a.W5;r.W6=a.W6+1;
  r.W7=a.W19;r.W8=a.W16;
  for(int word=0;word<32;word++)r.SetWord(10+word,a.Word(12+word));
  r.W17=a.W19+1;r.W14=r.Checksum(10,32);r.W4=r.Checksum(0,42);
  if(UGameplayStatics.SaveGameToSlot(r,"MCD2GraphicsProviderRequest",0)){
   SentSession=a.W5;SentSequence=r.W6;LoggedSequence=0;
   Log.Write("PROVIDER TRANSPORT REQUEST session="+r.W5+" sequence="+r.W6+" base="+r.W7);
  }
 }
}
