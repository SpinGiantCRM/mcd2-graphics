"""Compile the actual Actor lease methods against a tiny console/runtime fake.

Run with --dotnet pointing to the UI build's SDK; no game or save files touched.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def method(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dotnet', default='dotnet')
    args = parser.parse_args()
    source = (Path(__file__).resolve().parents[1] / 'src/ui/ModActor.cs').read_text()
    fields = next(line for line in source.splitlines() if 'bool foliageVelocityOwned;' in line)
    methods = '\n'.join(method(source, name) for name in (
        'public void RestoreFoliageVelocity()', 'public void UpdateFoliageVelocity(int ready)'))
    assert 'RestoreFoliageVelocity();' in method(source, 'protected override void ReceiveEndPlay(')
    harness = r'''
using System;
class FGSettingsSave {public int Mode=1,SessionId=42;}
class FGRuntimeSave {public int Available=1,SessionId=42,RestartRequired=0,Phase=3,Active=1;}
class GraphicsSettingsSave {public int SchemaVersion=4,Revision=7,RenderContextSessionId=42,RenderContextReady=1,ReconstructionMode=2;}
class GraphicsRuntimeStateSave {public int SchemaVersion=1,RequestedRevision=7,SessionId=42,ErrorCode=0,Phase=2;}
class DisplaySettingsSave {public int HDROutput=1;}
class ProviderMenuClient {}
class ProviderRuntimeClient {public bool Eligible=true,Active=true;public bool FoliageMotionReady(ProviderMenuClient m,bool acquire)=>Eligible && (!acquire || Active);}
static class UGameplayStatics {public static object GetPlayerController(object x,int id)=>x;}
static class Log {public static void Write(string x){}}
static class UKismetSystemLibrary {
 public static int Vertex=2,Pass=2,Commands,Reads;public static bool Exists=true,Fail=false;
 public static string GetConsoleVariableStringValue(string n){Reads++;return Exists?"value":"";}
 public static int GetConsoleVariableIntValue(string n){Reads++;return n=="r.VelocityOutputPass"?Pass:Vertex;}
 public static void ExecuteConsoleCommand(object x,string cmd,object p){Commands++;if(!Fail)Vertex=int.Parse(cmd[(cmd.LastIndexOf(' ')+1)..]);}
 public static void Reset(int v=2){Vertex=v;Pass=2;Commands=Reads=0;Exists=true;Fail=false;}
}
class ModActor {
 public FGSettingsSave FGSaved=new();public FGRuntimeSave FGRuntime=new();
 public GraphicsSettingsSave Saved=new();public GraphicsRuntimeStateSave Runtime=new();public DisplaySettingsSave DisplaySaved=new();
 public bool ProviderOwned,SharedReady=true;public int Selected=2;
 public ProviderMenuClient ProviderMenu=new();public ProviderRuntimeClient ProviderRuntime=new();
 public bool SharedControls()=>SharedReady;public int SelectedProvider()=>Selected;
__FIELDS__
__METHODS__
}
class Program {
 static int checks;
 static void Check(bool x,string msg){checks++;if(!x)throw new Exception(msg);}
 static ModActor New(int v=2){UKismetSystemLibrary.Reset(v);return new();}
 static void Main(){
  var a=New();a.FGSaved.Mode=0;a.FGRuntime.Available=0;a.DisplaySaved.HDROutput=0;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==1,"SR alone without FG or HDR");
  a=New();a.Saved.ReconstructionMode=1;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==1,"DLAA");
  a=New();a.Runtime.Phase=3;a.Runtime.ErrorCode=1;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"unsupported adapter/fallback");
  a=New();a.Runtime.SessionId=41;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"stale session");
  a=New();a.Runtime.RequestedRevision=6;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"stale request");
  a=New();a.Saved.SchemaVersion=3;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"old settings schema");
  a=New();a.Runtime.SchemaVersion=9;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"unknown runtime schema");
  a=New();a.Saved.ReconstructionMode=0;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"native SR");
  a=New();a.UpdateFoliageVelocity(0);Check(UKismetSystemLibrary.Commands==0,"do not acquire in menus");
  a=New();a.Runtime.Phase=1;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"pending source");
  foreach(int original in new[]{0,2}){
   a=New(original);a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==1 && UKismetSystemLibrary.Commands==1,"acquire");
   a.UpdateFoliageVelocity(0);Check(UKismetSystemLibrary.Vertex==1 && UKismetSystemLibrary.Commands==1,"retain through menu");
   a.Saved.ReconstructionMode=0;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==original && UKismetSystemLibrary.Commands==2,"restore original");
   a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==2,"restore once");
  }
  a=New(1);a.UpdateFoliageVelocity(1);a.Saved.ReconstructionMode=0;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==1 && UKismetSystemLibrary.Commands==0,"preexisting value unowned");
  a=New();a.UpdateFoliageVelocity(1);UKismetSystemLibrary.Vertex=0;a.UpdateFoliageVelocity(1);a.UpdateFoliageVelocity(1);a.Saved.ReconstructionMode=0;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==0 && UKismetSystemLibrary.Commands==1,"external change respected");
  a.Saved.ReconstructionMode=2;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==1,"new intent can reacquire");a.RestoreFoliageVelocity();Check(UKismetSystemLibrary.Vertex==0,"EndPlay restore");
  a=New();UKismetSystemLibrary.Fail=true;a.UpdateFoliageVelocity(1);for(int i=0;i<10;i++)a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==1,"failed apply does not spam");
  a.Saved.ReconstructionMode=0;a.UpdateFoliageVelocity(1);UKismetSystemLibrary.Fail=false;a.Saved.ReconstructionMode=2;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==1,"retry after fresh intent");
  a=New();UKismetSystemLibrary.Pass=1;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"untested output pass");
  a=New();UKismetSystemLibrary.Exists=false;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"missing variable");
  a=New(9);a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"unknown mode");
  a=New();a.UpdateFoliageVelocity(1);UKismetSystemLibrary.Fail=true;a.Saved.ReconstructionMode=0;for(int i=0;i<10;i++)a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==4,"restore retries bounded");
  a=New();a.UpdateFoliageVelocity(1);a.FGSaved.Mode=0;a.FGRuntime.Active=0;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==1,"FG Off keeps active SR lease");
  a=New();a.UpdateFoliageVelocity(1);a.Runtime.Phase=3;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==2,"runtime fault restores");
  a=New();a.UpdateFoliageVelocity(1);a.Saved.RenderContextReady=0;a.UpdateFoliageVelocity(0);Check(UKismetSystemLibrary.Vertex==2,"travel restores");
  a=New();a.ProviderOwned=true;a.Saved.ReconstructionMode=0;a.Runtime=null;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==1,"FSR acknowledgement acquires without DLSS acknowledgement");
  a.UpdateFoliageVelocity(0);Check(UKismetSystemLibrary.Vertex==1,"FSR temporary menu retains");
  a.ProviderRuntime.Active=false;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==1,"FSR transient evaluation retains");
  a.ProviderRuntime.Eligible=false;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==2,"FSR loss restores");
  a=New();a.ProviderOwned=true;a.ProviderRuntime.Active=false;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"FSR pending cannot acquire");
  a=New();a.ProviderOwned=true;a.ProviderRuntime.Eligible=false;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==0,"stale DLSS acknowledgement cannot admit selected FSR");
  a=New();a.ProviderOwned=true;a.UpdateFoliageVelocity(1);a.SharedReady=false;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==2,"lost authority restores");
  a=New();a.ProviderOwned=true;a.UpdateFoliageVelocity(1);a.Selected=0;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Vertex==2,"shared Native selection restores");
  a=New();a.ProviderOwned=true;a.Selected=1;a.UpdateFoliageVelocity(1);a.Selected=2;a.UpdateFoliageVelocity(1);Check(UKismetSystemLibrary.Commands==1,"DLSS to acknowledged FSR shares the existing lease");a.RestoreFoliageVelocity();Check(UKismetSystemLibrary.Vertex==2,"shared lease restores original");
  Console.WriteLine($"{checks} foliage lease checks passed");
 }
}
'''.replace('__FIELDS__', fields).replace('__METHODS__', methods)
    with tempfile.TemporaryDirectory(prefix='mcd2-foliage-policy-') as directory:
        root = Path(directory)
        (root/'Program.cs').write_text(harness)
        (root/'Policy.csproj').write_text('<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework></PropertyGroup></Project>')
        subprocess.run([args.dotnet, 'run', '--project', str(root/'Policy.csproj'), '--verbosity', 'quiet'], check=True)


if __name__ == '__main__':
    main()
