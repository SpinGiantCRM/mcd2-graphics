using NeoRune;
using MCD2Graphics;
using UE.CoreUObject;
using UE.Engine;
using UE.UMG;
using UE.SlateCore;
public class RuntimeDiagnosticsOverlay : ScreenWidget {
 public UTextBlock? Label;
 public override void OnInitialized(){
  bIsFocusable=false;var tree=Ui.Tree(this);if(tree==null)return;
  Label=Ui.Text(tree,"Waiting for runtime activity",18);
  tree.RootWidget=Ui.Panel(tree,Label,Ui.Color(0,0,0,0.75f),12);
  SetVisibility(ESlateVisibility.HitTestInvisible);
 }
 public static RuntimeDiagnosticsOverlay? Open(ModActor manager){
  var view=Ui.Create(manager,Unreal.ClassOf<RuntimeDiagnosticsOverlay>()) as RuntimeDiagnosticsOverlay;
  if(view!=null)view.ShowAt(new FVector2D{X=24,Y=24},new FVector2D{X=0,Y=0},ScreenWidget.UnderGameUI);return view;
 }
}
