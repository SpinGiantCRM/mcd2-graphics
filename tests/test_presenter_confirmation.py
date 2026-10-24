"""Execute the actual actor confirmation restore method against settings fakes."""
import argparse
from pathlib import Path
import subprocess
import tempfile

from test_fg_foliage_policy import method


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dotnet', default='dotnet')
    args = parser.parse_args()
    source = (Path(__file__).resolve().parents[1] / 'src/ui/ModActor.cs').read_text()
    restore = method(source, 'public void RestorePresenterConfirmation(bool final)')
    display = method(source, 'public void RestorePresenterDisplay()')
    focus = method(source, 'public void RestorePresenterFocus()')
    begin = method(source, 'public void PollPresenterSwitch()')
    end = method(source, 'protected override void ReceiveEndPlay(')
    assert display.index('settings.ApplyResolutionSettings(false)') < display.index('RestorePresenterConfirmation(false)')
    assert focus.index('RestorePresenterConfirmation(true)') < focus.index('!PresenterRestoreFGFocus')
    assert begin.index('PresenterOriginalConfirmedX=') < begin.index('settings.SetScreenResolution(PresenterTemporaryResolution)')
    assert begin.index('RestorePresenterConfirmation(true);Timer.Stop(this,"RestorePresenterFocus")') < begin.index('PresenterOriginalMode=mode')
    assert 'RestorePresenterDisplay();RestorePresenterConfirmation(true);' in end
    harness = r'''
using System;
enum EWindowMode {Fullscreen, WindowedFullscreen, Windowed}
struct FIntPoint {public int X,Y; public FIntPoint(int x,int y){X=x;Y=y;}}
class UGameUserSettings {
 public static UGameUserSettings? Current;
 public static UGameUserSettings? GetGameUserSettings()=>Current;
 public EWindowMode Mode=EWindowMode.Windowed;
 public FIntPoint Resolution=new(3840,2160);
 public uint LastUserConfirmedResolutionSizeX=3824,LastUserConfirmedResolutionSizeY=2144;
 public int LastConfirmedFullscreenMode=2,Saves;
 public EWindowMode GetFullscreenMode()=>Mode;
 public FIntPoint GetScreenResolution()=>Resolution;
 public FIntPoint GetLastConfirmedScreenResolution()=>new((int)LastUserConfirmedResolutionSizeX,(int)LastUserConfirmedResolutionSizeY);
 public void SaveSettings(){Saves++;}
}
class ModActor {
 EWindowMode PresenterOriginalMode,PresenterTemporaryMode;
 FIntPoint PresenterOriginalResolution,PresenterTemporaryResolution;
 uint PresenterOriginalConfirmedX,PresenterOriginalConfirmedY;
 int PresenterOriginalConfirmedMode;bool PresenterConfirmationPending;
 public bool Pending=>PresenterConfirmationPending;
 public static ModActor Create(EWindowMode mode=EWindowMode.Windowed){
  UGameUserSettings.Current=new(){Mode=mode};return new(){PresenterOriginalMode=mode,PresenterTemporaryMode=EWindowMode.Windowed,
   PresenterOriginalResolution=new(3840,2160),PresenterTemporaryResolution=new(3824,2144),
   PresenterOriginalConfirmedX=3200,PresenterOriginalConfirmedY=1800,PresenterOriginalConfirmedMode=1,PresenterConfirmationPending=true};
 }
__METHOD__
}
class Program {
 static int checks;
 static void Check(bool value,string name){checks++;if(!value)throw new Exception(name);}
 static void Main(){
  foreach(var mode in new[]{EWindowMode.Windowed,EWindowMode.WindowedFullscreen}){
   var a=ModActor.Create(mode);var s=UGameUserSettings.Current!;
   a.RestorePresenterConfirmation(false);
   Check(s.LastUserConfirmedResolutionSizeX==3200&&s.LastUserConfirmedResolutionSizeY==1800&&s.LastConfirmedFullscreenMode==1,"exact previous confirmation restored");
   Check(s.Saves==1&&a.Pending,"immediate restore retains bounded deferred check");
   a.RestorePresenterConfirmation(false);Check(s.Saves==1,"unchanged values are not saved repeatedly");
   a.RestorePresenterConfirmation(true);Check(!a.Pending&&s.Saves==1,"deferred check ends ownership");
   s.LastUserConfirmedResolutionSizeX=3824;s.LastUserConfirmedResolutionSizeY=2144;s.LastConfirmedFullscreenMode=2;
   a.RestorePresenterConfirmation(true);Check(s.Saves==1,"unowned settings are untouched");
  }
  {
   var a=ModActor.Create();var s=UGameUserSettings.Current!;
   a.RestorePresenterConfirmation(false);s.LastUserConfirmedResolutionSizeX=3824;s.LastUserConfirmedResolutionSizeY=2144;s.LastConfirmedFullscreenMode=2;
   a.RestorePresenterConfirmation(true);Check(s.Saves==2&&!a.Pending&&s.LastUserConfirmedResolutionSizeX==3200,"late engine confirmation is restored once");
  }
  for(int change=0;change<4;change++){
   var a=ModActor.Create();var s=UGameUserSettings.Current!;
   if(change==0)s.Resolution=new(2560,1440);
   if(change==1)s.Mode=EWindowMode.WindowedFullscreen;
   if(change==2)s.LastUserConfirmedResolutionSizeX=2560;
   if(change==3)s.LastConfirmedFullscreenMode=1;
   a.RestorePresenterConfirmation(true);Check(s.Saves==0&&!a.Pending,"later user edit wins "+change);
  }
  {
   var a=ModActor.Create();var s=UGameUserSettings.Current!;a.RestorePresenterConfirmation(false);
   s.LastUserConfirmedResolutionSizeX=1920;s.LastUserConfirmedResolutionSizeY=1080;
   a.RestorePresenterConfirmation(true);Check(s.Saves==1&&s.LastUserConfirmedResolutionSizeX==1920,"later confirmation after immediate restore wins");
  }
  {
   var a=ModActor.Create();UGameUserSettings.Current=null;
   a.RestorePresenterConfirmation(false);Check(a.Pending,"missing settings allow deferred check");
   a.RestorePresenterConfirmation(true);Check(!a.Pending,"missing settings finish bounded ownership");
  }
  Console.WriteLine($"{checks} presenter confirmation checks passed");
 }
}
'''.replace('__METHOD__', restore)
    with tempfile.TemporaryDirectory(prefix='mcd2-presenter-confirmation-') as temp:
        folder = Path(temp)
        (folder / 'Program.cs').write_text(harness)
        (folder / 'Test.csproj').write_text('<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework><Nullable>enable</Nullable><TreatWarningsAsErrors>true</TreatWarningsAsErrors></PropertyGroup></Project>')
        subprocess.run([args.dotnet, 'run', '--project', str(folder / 'Test.csproj'), '-c', 'Release'], check=True)


if __name__ == '__main__':
    main()
