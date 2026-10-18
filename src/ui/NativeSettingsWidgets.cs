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
 // Reuse the native label, focus response and amber selection glow without
 // a copied main-page backdrop or screen-specific tab controller.
 public static UAS_SpicewoodButtonTabAction? CategoryTab(UObject owner,string title){
  var tab=Ui.GameWidget(owner,"/OreUI/UI/Button/W_SpicewoodTabButton.W_SpicewoodTabButton_C") as UAS_SpicewoodButtonTabAction;if(tab==null)return null;
  tab.SetButtonText(title);tab.PreConstruct(false);tab.UpdateButtonText(title);
  if(tab.ButtonTemplate!=null){tab.ButtonTemplate.bIsButtonTextCentered=true;tab.ButtonTemplate.PreConstruct(false);}
  SetText(tab.TextBlockWidget,title);return tab;
 }
 public static USizeBox? CategoryDivider(UWidgetTree tree){
  var line=UE.Engine.UGameplayStatics.SpawnObject(Unreal.ClassOf<USizeBox>(),tree) as USizeBox;
  var ink=Ui.Panel(tree,null,Ui.Color(0.035f,0.065f,0.075f,0.65f),0);if(line==null||ink==null)return null;
  line.SetHeightOverride(1);line.AddChild(ink);line.SetVisibility(ESlateVisibility.HitTestInvisible);return line;
 }
 // The frame is owned by W_SettingsPanel, not W_SettingsDetailView. Reparent
 // only the newly created shell's images; never touch a live game settings page.
 public static UOverlay? HelpFrame(UWidgetTree tree,UE.SpicewoodSettings.Spicewood.UI.SettingsScreen.W_SettingsPanel.UW_SettingsPanel_C? shell,UGameSettingDetailView? details){
  var frame=UE.Engine.UGameplayStatics.SpawnObject(Unreal.ClassOf<UOverlay>(),tree) as UOverlay;if(frame==null)return null;
  if(shell!=null){AddFrameLayer(frame,shell.BG);AddFrameLayer(frame,shell.Deco);}
  if(details!=null){StretchHelpBody(details);details.RemoveFromParent();var slot=frame.AddChildToOverlay(details);if(slot!=null){slot.SetHorizontalAlignment(UE.SlateCore.EHorizontalAlignment.HAlign_Fill);slot.SetVerticalAlignment(UE.SlateCore.EVerticalAlignment.VAlign_Fill);slot.SetPadding(Ui.Margin(30));}}
  return frame;
 }
 // Grow the description branch, keeping the native recommendation extension
 // at the bottom of this newly created help card. Live game pages are untouched.
 public static void StretchHelpBody(UGameSettingDetailView details){
  var description=details.RichText_Description;var footer=details.Box_DetailsExtension;if(description==null||footer==null)return;
  UWidget? ancestor=footer.GetParent();for(int depth=0;ancestor!=null&&depth<32;depth++,ancestor=ancestor.GetParent()){
   var column=ancestor as UVerticalBox;if(column==null)continue;
   UWidget? body=description;for(int i=0;body!=null&&body.GetParent()!=column&&i<32;i++)body=body.GetParent();
   UWidget? bottom=footer;for(int i=0;bottom!=null&&bottom.GetParent()!=column&&i<32;i++)bottom=bottom.GetParent();
   if(body==null||bottom==null||body==bottom)continue;
   var bodySlot=body.Slot as UVerticalBoxSlot;var footerSlot=bottom.Slot as UVerticalBoxSlot;if(bodySlot==null||footerSlot==null)continue;
   bodySlot.SetSize(new UE.UMG.FSlateChildSize{SizeRule=UE.UMG.ESlateSizeRule.Fill,Value=1});footerSlot.SetSize(new UE.UMG.FSlateChildSize{SizeRule=UE.UMG.ESlateSizeRule.Automatic,Value=1});return;
  }
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
