from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class MultiplierUI(unittest.TestCase):
 def test_actual_actor_count_controls_and_device_gates(self):
  source=(ROOT/"src/ui/ModActor.cs").read_text()
  methods=source[source.index(" public int FgDlssGenerated()"):source.index(" public int DisplayValue",source.index(" public int FgDlssGenerated()"))]
  policy=(ROOT/"src/ui/AdvancedSettings.cs").read_text()
  fixture=r'''#nullable enable
using System;
using System.Collections.Generic;
class Authority {public int W5=77;}
class Menu {public Authority? Authority=new();public int D=1,F,Order;public int Value(int word)=>word==20?(D>0?0:1):word==22?D+F+1:0;public int FrameOrder()=>Order;public bool SelectFgOrder(int code){Order=code;return true;}public int DlssGenerated()=>D;public int FsrGenerated()=>F;public bool SelectFgAllocation(int d,int f){D=d;F=f;Order=0;return true;}}
class State {public int SessionId=77,Available=1,AvailableProviders=3,SoftwareMultiplierMask=124,MaximumGeneratedFrames=5,MfgNativeGeneratedMax=5,StartupOwner=4;}
class Display {public int SchemaVersion=1,SessionId=77,RenderingVendorId=0x10de;}
class Actor {
 public bool Shared=true;public Menu? ProviderMenu=new();public State? FGRuntime=new();public Display? DisplayRuntime=new();public int Sr=1,Enabled;
 public bool SharedControls()=>Shared;public int SelectedFgProvider()=>ProviderMenu!.Value(20);public int SelectedProvider()=>Sr;public int DisplayValue(int id)=>id==8?0:Enabled;public void RefreshDisplayRows(){}
 METHODS
}
class Program {static void Need(bool ok){if(!ok)throw new Exception("actual actor allocation failed");}static void Main(){
 var a=new Actor();Need(a.SelectedFgAvailable());a.ChangeFgCount(false,1);a.ChangeFgCount(false,1);a.ChangeFgCount(false,1);a.ChangeFgCount(true,1);
 Need(a.FgDlssGenerated()==2&&a.FgFsrGenerated()==3&&a.Enabled==0&&a.FgAllocationSummary().Contains("6x"));
 a.ChangeFgOrder(1);Need(a.ProviderMenu!.Order!=0&&a.FgDlssGenerated()==2&&a.FgFsrGenerated()==3&&a.Enabled==0);
 Need(a.FgOrderSummary().Contains("Real"));a.ChangeFgCount(true,1);Need(a.FgDlssGenerated()+a.FgFsrGenerated()<=5&&a.ProviderMenu.Order==0);
 var unchanged=new Actor();unchanged.FGRuntime!.SoftwareMultiplierMask=0;unchanged.FGRuntime.AvailableProviders=1;unchanged.FGRuntime.MaximumGeneratedFrames=1;unchanged.FGRuntime.MfgNativeGeneratedMax=0;unchanged.ProviderMenu!.Order=33;unchanged.ChangeFgCount(true,1);Need(unchanged.ProviderMenu.Order==33);
 a.ProviderMenu.D=2;a.ProviderMenu.F=3;a.ProviderMenu.Order=35;a.FGRuntime!.MfgNativeGeneratedMax=2;Need(!a.SelectedFgAvailable()&&a.FgOrderLabel(35).Contains("Unavailable"));

 a.DisplayRuntime!.RenderingVendorId=0x1002;Need(!a.FgAllocationSupported(1,1));a.ProviderMenu!.D=0;a.ProviderMenu.F=1;a.Sr=2;a.ChangeFgCount(false,1);Need(a.FgFsrGenerated()==2);a.ChangeFgCount(true,1);Need(a.FgDlssGenerated()==0);
 a.FGRuntime!.SessionId=76;Need(!a.SelectedFgAvailable());a.FGRuntime.SessionId=77;a.DisplayRuntime.SessionId=76;Need(!a.FgAllocationSupported(1,0));
 Console.WriteLine("Actual actor: independent counts, total6x bound, no implicit FG enable, AMD DLSS refusal and stale-session gates pass");
}}
'''.replace("METHODS",methods)+"\n"+policy[policy.index("public static class MfgModes"):]
  with tempfile.TemporaryDirectory(prefix="mcd2-fg-counts-") as d:
   path=Path(d);(path/"Program.cs").write_text(fixture);(path/"Fixture.csproj").write_text('<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework></PropertyGroup></Project>')
   result=subprocess.run(["dotnet","run","--project",str(path/"Fixture.csproj"),"-c","Release","--verbosity","quiet"],capture_output=True,text=True,timeout=120)
   self.assertEqual(result.returncode,0,result.stdout+result.stderr)
if __name__=="__main__":unittest.main()
