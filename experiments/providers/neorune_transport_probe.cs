using NeoRune;
using UE.Engine;

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
