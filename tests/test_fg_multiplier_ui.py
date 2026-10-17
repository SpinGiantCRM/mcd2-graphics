from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]

class MultiplierUI(unittest.TestCase):
 def test_real_selector_caps_legacy_persistence_and_shared_request(self):
  source=(ROOT/"src/ui/ModActor.cs").read_text(encoding="utf-8")
  start=source.index(" public bool NvidiaFrameModeAvailable()")
  end=source.index(" }if(id==6",start)
  methods=source[start:end]+" }}"
  fixture=r'''#nullable enable
using System;
class Save {public int FrameMode,Revision;}
class State {public int MaximumGeneratedFrames,DynamicSupported;}
class Menu {public int Strategy,Multiplier;public bool Enabled;public void SelectFg(int p,int s,int m,bool e){Strategy=s;Multiplier=m;Enabled=e;}}
class Selector {
 public Save? FGSaved=new();public State? FGRuntime=new();public Menu? ProviderMenu=new();
 public bool ProviderOwned,Available=true;public int Provider,Persisted;
 bool SharedControls()=>true;int SelectedFgProvider()=>Provider;bool SelectedFgAvailable()=>Available;
 int DisplayValue(int id)=>id==8?FGSaved!.FrameMode:1;void PersistFG(){Persisted++;}void RefreshDisplayRows(){}
METHODS
}
class Program {
 static void Check(bool v){if(!v)throw new Exception("Multiplier selector failed");}
 static void Main(){
  var s=new Selector();Check(!s.NvidiaFrameModeAvailable());s.FGRuntime!.MaximumGeneratedFrames=1;
  Check(s.FrameModeSupported(0)&&!s.FrameModeSupported(1)&&!s.FrameModeSupported(5)&&!s.FrameModeSupported(6));
  s.ChangeDisplay(8,1);Check(s.FGSaved!.FrameMode==0&&s.Persisted==0);
  s.FGRuntime.MaximumGeneratedFrames=5;s.FGRuntime.DynamicSupported=1;
  for(int mode=1;mode<=5;mode++){s.ChangeDisplay(8,1);Check(s.FGSaved.FrameMode==mode);}
  s.ChangeDisplay(8,1);Check(s.FGSaved.FrameMode==0&&s.Persisted==6);
  s.ChangeDisplay(8,-1);Check(s.FGSaved.FrameMode==5);
  s.FGRuntime.MaximumGeneratedFrames=1;s.FGRuntime.DynamicSupported=0;s.ChangeDisplay(8,1);Check(s.FGSaved.FrameMode==0);
  s.FGRuntime.MaximumGeneratedFrames=5;s.FGRuntime.DynamicSupported=1;s.ProviderOwned=true;
  s.ChangeDisplay(8,1);Check(s.ProviderMenu!.Strategy==1&&s.ProviderMenu.Multiplier==3&&s.ProviderMenu.Enabled);
  s.FGSaved.FrameMode=4;s.ChangeDisplay(8,1);Check(s.ProviderMenu.Strategy==3&&s.ProviderMenu.Multiplier==2);
  s.Provider=1;Check(!s.NvidiaFrameModeAvailable());int persisted=s.Persisted;s.ChangeDisplay(8,1);Check(s.Persisted==persisted);
 }
}
'''.replace("METHODS",methods)
  with tempfile.TemporaryDirectory(prefix="mcd2-mfg-ui-") as d:
   path=Path(d);(path/"Program.cs").write_text(fixture,encoding="utf-8")
   (path/"Fixture.csproj").write_text('<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework></PropertyGroup></Project>')
   result=subprocess.run(["dotnet","run","--project",str(path/"Fixture.csproj"),"-c","Release","--verbosity","quiet"],capture_output=True,text=True,timeout=120)
   self.assertEqual(result.returncode,0,result.stdout+result.stderr)

if __name__=="__main__":unittest.main()
