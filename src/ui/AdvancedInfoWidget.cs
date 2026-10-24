using NeoRune;
using MCD2Graphics;
using System.Collections.Generic;
using UE.CoreUObject;
using UE.Engine;
using UE.UMG;
using UE.Angelscript;

public class AdvancedInfoWidget : ModWidget {
 public UTextBlock? Label;
 public ModActor? Manager;bool observing;
 protected override UWidget? Build(UWidgetTree tree){
  Label=Ui.Text(tree,"Waiting for runtime connection.",24);
  var skin=Ui.GameWidget(this,"/SpicewoodSettings/Spicewood/UI/SettingsScreen/Editors/W_SettingsEntry_Stepper.W_SettingsEntry_Stepper_C") as UAS_SettingsEntry_Stepper;
  if(Label!=null){if(skin!=null)Ui.StyleLike(Label,skin.Text_SettingName);Label.SetAutoWrapText(true);Label.SetVisibility(UE.UMG.ESlateVisibility.Visible);
  }
  return Label;
 }
 public override void Construct(){
  UGameplayStatics.GetAllActorsOfClass(this,Unreal.ClassOf<ModActor>(),out List<AActor> actors);
  if(actors.Count>0)Manager=actors[0] as ModActor;

  RefreshInfo();Timer.Start(this,"RefreshInfo",0.5f,true);
 }
 public override void Destruct(){Timer.Stop(this,"RefreshInfo");if(observing&&Manager!=null&&Manager.DiagnosticObservers>0)Manager.DiagnosticObservers--;observing=false;Manager=null;}
 public void RefreshInfo(){
  if(Label==null||Manager==null||Manager.Advanced==null)return;var a=Manager.Advanced;
  if(!Manager.ShowMenuDiagnostics){Label.SetText("");Label.SetVisibility(UE.UMG.ESlateVisibility.Collapsed);if(observing&&Manager.DiagnosticObservers>0)Manager.DiagnosticObservers--;observing=false;return;}
  Label.SetVisibility(UE.UMG.ESlateVisibility.Visible);if(!observing){Manager.DiagnosticObservers++;observing=true;}
  var d=Manager.Diagnostics==null?null:Manager.Diagnostics.State;var fg=Manager.AdvancedFG;
  Label.SetText(a.Status()+"\nDLSS SR version: "+a.Version(0)+"\n\nDLSS FG preset (requested): "+(fg==null?"Waiting":fg.Hint==1?"A - Standard":fg.Hint==2?"B - UI Recomposition":fg.Hint==3?"C - Performance":"Auto")+
   "\nDLSS FG driver request: "+(fg!=null&&fg.State!=null&&fg.Hint!=fg.State.W11?"Restart to apply":d==null?"Waiting":d.FgPresetStatus==1?"Accepted (model not reported)":d.FgPresetStatus==-20||d.FgPresetStatus==-21?"Requires a dedicated per-game NVIDIA profile":d.FgPresetStatus<0?"Not applied":"SDK default / unchanged")+"\nDLSS FG version: "+a.Version(4)+"\nActive DLSS models: not exposed by the runtime."+
   "\n\nFSR SR implementation (last verified): "+RuntimeDiagnosticsClient.Implementation(d==null?0:d.FsrSrVersion)+"\nFSR SR library: "+a.Version(2)+
   "\nFSR FG implementation (last verified): "+RuntimeDiagnosticsClient.Implementation(d==null?0:d.FsrFgVersion)+"\nFSR FG library: "+a.Version(3)+"\nStreamline version: "+a.Version(1));
 }
}
