// MIT licensed reusable helpers for Blueprint Loader custom settings widgets.
// References the game's existing widgets; no game assets are redistributed.
using NeoRune;
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
 // Keep the native strip's gradient, border and layout instead of rebuilding
 // its background with a flat colour. Q/E remain owned by the game's main tabs.
 public static UE.SpicewoodUI.USpicewoodTabListWidget? CategoryStrip(UObject owner){
  var strip=Ui.GameWidget(owner,"/OreUI/UI/Widget/TabList/W_SpicewoodTabList.W_SpicewoodTabList_C") as UE.SpicewoodUI.USpicewoodTabListWidget;
  if(strip==null)return null;strip.SetListeningForInput(false);strip.SetTabBackgroundVisibility(true);strip.SetTabActionPromptVisibility(false);
  if(strip.PreviousTabAction!=null)strip.PreviousTabAction.SetVisibility(ESlateVisibility.Collapsed);
  if(strip.NextTabAction!=null)strip.NextTabAction.SetVisibility(ESlateVisibility.Collapsed);
  if(strip.TabButtonBox!=null)strip.TabButtonBox.ClearChildren();return strip;
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
