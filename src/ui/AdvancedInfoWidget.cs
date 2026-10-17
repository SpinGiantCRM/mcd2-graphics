using NeoRune;
using MCD2Graphics;
using System.Collections.Generic;
using UE.CoreUObject;
using UE.Engine;
using UE.UMG;

public class AdvancedInfoWidget : ModWidget {
 public UTextBlock? Label;
 protected override UWidget? Build(UWidgetTree tree){Label=Ui.Text(tree,"Waiting for settings connection.",22);if(Label!=null)Label.SetAutoWrapText(true);return Label;}
 public override void Construct(){RefreshInfo();Timer.Start(this,"RefreshInfo",0.5f,true);}
 public override void Destruct(){Timer.Stop(this,"RefreshInfo");}
 public void RefreshInfo(){
  if(Label==null)return;
  UGameplayStatics.GetAllActorsOfClass(this,Unreal.ClassOf<ModActor>(),out List<AActor> actors);
  if(actors.Count==0)return;var manager=actors[0] as ModActor;
  if(manager!=null && manager.Advanced!=null)Label.SetText(manager.Advanced.Summary());
 }
}
