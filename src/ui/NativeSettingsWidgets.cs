// MIT licensed reusable helpers for Blueprint Loader custom settings widgets.
// References the game's existing widgets; no game assets are redistributed.
using NeoRune;
using System.Collections.Generic;
using UE.CoreUObject;
using UE.UMG;
using UE.GameSettings;
using UE.Angelscript;
public static class NativeSettingsWidgets {
 public static UGameSettingDetailView? HelpCard(UObject owner){return Ui.GameWidget(owner,"/SpicewoodSettings/Spicewood/UI/SettingsScreen/W_SettingsDetailView.W_SettingsDetailView_C") as UGameSettingDetailView;}
 public static UAS_SpicewoodButtonTabAction? CategoryTab(UObject owner,string title){
  var tab=Ui.GameWidget(owner,"/OreUI/UI/Button/W_SpicewoodTabButton.W_SpicewoodTabButton_C") as UAS_SpicewoodButtonTabAction;if(tab==null)return null;
  tab.SetButtonText(title);tab.PreConstruct(false);tab.UpdateButtonText(title);
  if(tab.ButtonTemplate!=null){tab.ButtonTemplate.bIsButtonTextCentered=true;tab.ButtonTemplate.PreConstruct(false);}
  SetText(tab.TextBlockWidget,title);return tab;
 }
 // A native strip controller expects screen-specific bindings and registration.
 // Borrow only its display brush from an initialized Settings screen. Do not
 // instantiate the controller or call its optional-child visibility methods.
 public static bool CopyCategoryStripStyle(UObject owner,UImage? target){
  if(target==null)return false;
  UWidgetBlueprintLibrary.GetAllWidgetsOfClass(owner,out List<UUserWidget> screens,Unreal.ClassOf<UE.SpicewoodSettings.Spicewood.UI.SettingsScreen.W_SettingsScreen_Activatable.UW_SettingsScreen_Activatable_C>(),false);
  foreach(var screen in screens){if(!screen.IsVisible()||screen.WidgetTree==null)continue;var source=FindCategoryStripImage(screen.WidgetTree.RootWidget,0);if(source==null||source.Brush.ResourceObject==null)continue;target.SetBrush(source.Brush);target.SetColorAndOpacity(source.ColorAndOpacity);target.SetVisibility(ESlateVisibility.HitTestInvisible);return true;}
  return false;
 }
 public static UImage? FindCategoryStripImage(UWidget? widget,int depth){
  if(widget==null||depth>32)return null;
  var strip=widget as UE.SpicewoodUI.USpicewoodTabListWidget;if(strip!=null&&strip.TabButtonBoxBackground!=null)return strip.TabButtonBoxBackground;
  var nested=widget as UUserWidget;if(nested!=null&&nested.WidgetTree!=null&&nested.WidgetTree.RootWidget!=widget){var image=FindCategoryStripImage(nested.WidgetTree.RootWidget,depth+1);if(image!=null)return image;}
  var panel=widget as UPanelWidget;if(panel!=null)for(int i=0;i<panel.GetChildrenCount();i++){var image=FindCategoryStripImage(panel.GetChildAt(i),depth+1);if(image!=null)return image;}
  return null;
 }
 // The frame is owned by W_SettingsPanel, not W_SettingsDetailView. Reparent
 // only the newly created shell's images; never touch a live game settings page.
 public static UOverlay? HelpFrame(UWidgetTree tree,UE.SpicewoodSettings.Spicewood.UI.SettingsScreen.W_SettingsPanel.UW_SettingsPanel_C? shell,UGameSettingDetailView? details){
  var frame=UE.Engine.UGameplayStatics.SpawnObject(Unreal.ClassOf<UOverlay>(),tree) as UOverlay;if(frame==null)return null;
  if(shell!=null){AddFrameLayer(frame,shell.BG);AddFrameLayer(frame,shell.Deco);}
  if(details!=null){details.RemoveFromParent();var slot=frame.AddChildToOverlay(details);if(slot!=null){slot.SetHorizontalAlignment(UE.SlateCore.EHorizontalAlignment.HAlign_Fill);slot.SetVerticalAlignment(UE.SlateCore.EVerticalAlignment.VAlign_Fill);slot.SetPadding(Ui.Margin(30));}}
  return frame;
 }
 static void AddFrameLayer(UOverlay frame,UWidget? layer){
  if(layer==null)return;layer.RemoveFromParent();layer.SetVisibility(UE.UMG.ESlateVisibility.HitTestInvisible);
  var slot=frame.AddChildToOverlay(layer);if(slot!=null){slot.SetHorizontalAlignment(UE.SlateCore.EHorizontalAlignment.HAlign_Fill);slot.SetVerticalAlignment(UE.SlateCore.EVerticalAlignment.VAlign_Fill);}
 }
 public static void SetText(UWidget? widget,string text){
  if(widget==null)return;var plain=widget as UTextBlock;if(plain!=null)plain.SetText(text);var rich=widget as URichTextBlock;if(rich!=null)rich.SetText(text);
  var panel=widget as UPanelWidget;if(panel!=null)for(int i=0;i<panel.GetChildrenCount();i++)SetText(panel.GetChildAt(i),text);
 }
}
