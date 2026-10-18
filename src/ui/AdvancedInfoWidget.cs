using NeoRune;
using MCD2Graphics;
using System.Collections.Generic;
using UE.CoreUObject;
using UE.Engine;
using UE.UMG;
using UE.Angelscript;

public class AdvancedInfoWidget : ModWidget {
 public UTextBlock? Label;
 protected override UWidget? Build(UWidgetTree tree){
  Label=Ui.Text(tree,"Waiting for settings connection.",32);
  var skin=Ui.GameWidget(this,"/SpicewoodSettings/Spicewood/UI/SettingsScreen/Editors/W_SettingsEntry_Stepper.W_SettingsEntry_Stepper_C") as UAS_SettingsEntry_Stepper;
  if(Label!=null){if(skin!=null)Ui.StyleLike(Label,skin.Text_SettingName);Label.SetAutoWrapText(true);}
  return Label;
 }
 public override void Construct(){RefreshInfo();Timer.Start(this,"RefreshInfo",0.5f,true);}
 public override void Destruct(){Timer.Stop(this,"RefreshInfo");}
 public void RefreshInfo(){
  if(Label==null)return;
  UGameplayStatics.GetAllActorsOfClass(this,Unreal.ClassOf<ModActor>(),out List<AActor> actors);
  if(actors.Count==0)return;var manager=actors[0] as ModActor;
  if(manager!=null && manager.Advanced!=null)Label.SetText(manager.Advanced.Summary());
 }
}
