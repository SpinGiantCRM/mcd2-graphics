using UE.Engine;
static class AllocationTest {
 static void Need(bool ok,string message){if(!ok)throw new Exception("FG allocation: "+message);}
 public static void Run(){
  Need(FgAllocation.Valid(2,3)&&!FgAllocation.Valid(3,3)&&!FgAllocation.Valid(0,0),"additive maximum6x");
  Need(FgAllocation.NativeMaximumNeeded(2,3)==2&&FgAllocation.NativeMaximumNeeded(1,4)==1&&FgAllocation.NativeMaximumNeeded(2,2)==4,"uniform native grid");
  Need(FgAllocation.Supported(2,3,1,1,124,5,2,4,0x10de),"2DLSS3FSR exact caps");
  Need(!FgAllocation.Supported(2,3,1,1,124,5,1,4,0x10de),"native component caps respected");
  Need(!FgAllocation.Supported(1,2,2,3,124,5,5,4,0x1002),"AMD never admits DLSS");
  Need(FgAllocation.Supported(0,5,2,2,124,0,0,4,0x1002),"AMD FSR6x");
  Need(FgAllocation.Supported(1,0,2,1,124,0,0,4,0x10de),"baseline DLSS restart request");
  Need(!FgAllocation.Supported(2,0,2,1,124,0,5,4,0x10de),"headless cap never authorizes Streamline native");
  var a=new ProviderAuthoritySave{W0=0x3244434d,W1=0x00315341,W2=1,W3=176,W5=321,W11=2,W12=0x3244434d,W13=0x00354947,W14=1,W15=128,W18=5,W19=7,W20=1,W21=1,W22=6700,W23=7700,W24=1,W25=6700,W26=7700,W27=1,W28=6700,W29=7700,W30=1,W34=2,W36=1,W38=1000,W39=203,W40=203};
  a.W16=a.Checksum(12,32);a.W4=a.Checksum(0,44);UGameplayStatics.Slots.Clear();UGameplayStatics.Slots["MCD2GraphicsProviderAuthority"]=a;
  var c=new ProviderMenuClient();c.Poll();Need(c.Ready&&c.DlssGenerated()==1&&c.FsrGenerated()==0,"legacy default2x");
  Need(c.SelectFgAllocation(2,3),"explicit split request");Need(c.Value(19)==0,"extra settings never enable FG");Need(c.Value(21)==2&&c.DlssGenerated()==2&&c.FsrGenerated()==3&&c.Value(22)==6,"coherent additive selection");
  c.Poll();var r=(ProviderRequestSave)UGameplayStatics.Slots["MCD2GraphicsProviderRequest"];
  Need(r.W16==7&&r.W11==0x00374947&&r.W31==(2|(2<<8)|(3<<16))&&c.RequestValid(r),"versioned atomic wire");
  r.W31|=1<<24;r.W14=r.Checksum(10,32);r.W4=r.Checksum(0,42);Need(!c.RequestValid(r),"reserved allocation bits refused");
  Console.WriteLine("FG allocation: additive totals, device gates, uniform native grids, schema5 preservation and schema7 atomic split pass");
 }
}
