using UE.Engine;
static class OrderTest {
 static void Need(bool ok,string why){if(!ok)throw new Exception("FG order: "+why);}
 public static void WriteTable(string path){
  var rows=new System.Collections.Generic.List<string>();for(int d=0;d<=5;d++)for(int f=0;f<=5;f++)if(FgAllocation.Valid(d,f))for(int code=0;code<64;code++)for(int cap=0;cap<=5;cap++)rows.Add(d+","+f+","+code+","+cap+","+FgAllocation.ResolveOrder(d,f,code,cap));System.IO.File.WriteAllLines(path,rows);
 }
 public static void Run(){
  Need(FgAllocation.ValidOrder(2,3,42)&&FgAllocation.ValidOrder(2,3,35),"all matching manual sequences");
  Need(!FgAllocation.ValidOrder(2,3,34)&&!FgAllocation.ValidOrder(1,1,48)&&!FgAllocation.ValidOrder(1,1,1),"count/slot/marker corruption");
  Need(FgAllocation.ResolveOrder(2,3,0,2)==10,"Automatic balanced example");
  Need(FgAllocation.ResolveOrder(2,3,35,2)==-1&&FgAllocation.ResolveOrder(2,3,35,5)==3,"manual exact native-grid limit");
  Need(FgAllocation.ResolveOrder(2,2,0,4)==10,"balanced both providers at5x");
  Need(FgAllocation.Sequence(10,2,3,true)=="Real → FSR → DLSS → FSR → DLSS → FSR → Real","full timeline");
  var a=new ProviderAuthoritySave{W0=0x3244434d,W1=0x00315341,W2=1,W3=176,W5=321,W11=2,W12=0x3244434d,W13=0x00364947,W14=1,W15=128,W18=6,W19=7,W20=1,W21=1,W22=6700,W23=7700,W24=1,W25=6700,W26=7700,W27=1,W28=6700,W29=7700,W30=1,W33=2|(2<<8)|(3<<16),W34=6,W36=1,W38=1000,W39=203,W40=203};
  a.W16=a.Checksum(12,32);a.W4=a.Checksum(0,44);UGameplayStatics.Slots.Clear();UGameplayStatics.Slots["MCD2GraphicsProviderAuthority"]=a;
  var c=new ProviderMenuClient();c.Poll();Need(c.Ready&&c.FrameOrder()==0,"schema6 Automatic migration");
  Need(c.SelectFgOrder(35)&&c.Value(19)==0&&c.DlssGenerated()==2&&c.FsrGenerated()==3,"order edit changes neither counts nor enable");
  c.Poll();var r=(ProviderRequestSave)UGameplayStatics.Slots["MCD2GraphicsProviderRequest"];
  Need(r.W16==7&&r.W11==0x00374947&&((r.W31>>24)&63)==35&&c.RequestValid(r),"order atomic schema7 persistence");
  c.SelectFg(0,2,6,true);Need(c.FrameOrder()==35,"toggle retains custom order");
  c.SelectFgAllocation(1,1);Need(c.FrameOrder()==0&&c.Value(19)==1,"count edit restoresAutomatic without changing enabled");
  r.W31|=1<<30;r.W14=r.Checksum(10,32);r.W4=r.Checksum(0,42);Need(!c.RequestValid(r),"reserved wire bits refused");
  c.SelectFgAllocation(1,0);c.SelectFgOrder(33);Need(c.FrameOrder()==33,"single-provider custom order selected");
  Need(c.ResetDefaults()&&c.FrameOrder()==0&&c.Value(19)==0,"Reset Defaults restores Automatic for unchanged single-DLSS counts");
  Console.WriteLine("FG order: Automatic/manual timeline, capabilities, schema6->7, atomic persistence, toggles and count-edit reset pass");
 }
}
