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
 // A mod-owned navigation row. Native fonts and restrained colour keep this
 // distinct from the main page tabs; no screen controller or asset lookup.
 public static UButton? CategoryButton(UWidgetTree tree,string title,out UTextBlock? label,out UBorder? marker){
  var button=UE.Engine.UGameplayStatics.SpawnObject(Unreal.ClassOf<UButton>(),tree) as UButton;
  label=Ui.Text(tree,title,22);marker=Ui.Panel(tree,null,Ui.Color(0.03f,0.065f,0.075f,1),0);
  var column=Ui.Column(tree);if(button==null||column==null)return button;button.IsFocusable=true;
  if(label!=null){var text=column.AddChildToVerticalBox(label);if(text!=null){text.SetHorizontalAlignment(UE.SlateCore.EHorizontalAlignment.HAlign_Center);text.SetVerticalAlignment(UE.SlateCore.EVerticalAlignment.VAlign_Center);text.SetPadding(new UE.SlateCore.FMargin{Left=16,Right=16,Top=14,Bottom=14});}}
  var line=UE.Engine.UGameplayStatics.SpawnObject(Unreal.ClassOf<USizeBox>(),tree) as USizeBox;
  if(line!=null&&marker!=null){line.SetHeightOverride(3);line.AddChild(marker);column.AddChild(line);marker.SetVisibility(ESlateVisibility.HitTestInvisible);}
  var content=button.AddChild(column) as UButtonSlot;if(content!=null){content.SetPadding(Ui.Margin(0));content.SetHorizontalAlignment(UE.SlateCore.EHorizontalAlignment.HAlign_Fill);content.SetVerticalAlignment(UE.SlateCore.EVerticalAlignment.VAlign_Fill);}
  SetCategorySelected(button,label,marker,false);return button;
 }
 static UE.SlateCore.FSlateBrush CategoryBrush(float r,float g,float b,float a){return new UE.SlateCore.FSlateBrush{DrawAs=UE.SlateCore.ESlateBrushDrawType.Image,TintColor=Ui.SlateColor(Ui.Color(r,g,b,a))};}
 public static void SetCategorySelected(UButton button,UTextBlock? label,UBorder? marker,bool selected){
  var style=button.WidgetStyle;style.Normal=selected?CategoryBrush(0.012f,0.035f,0.045f,0.9f):CategoryBrush(0,0,0,0);
  style.Hovered=CategoryBrush(0.035f,0.075f,0.085f,0.8f);style.Pressed=CategoryBrush(0.055f,0.10f,0.11f,0.9f);
  style.NormalPadding=Ui.Margin(0);style.PressedPadding=Ui.Margin(0);button.SetStyle(style);button.SetBackgroundColor(Ui.Color(1,1,1,1));
  if(label!=null)label.SetColorAndOpacity(Ui.SlateColor(selected?Ui.Color(1,0.93f,0.80f,1):Ui.Color(0.48f,0.59f,0.62f,1)));
  if(marker!=null)marker.SetBrushColor(selected?Ui.Color(1,0.36f,0.006f,1):Ui.Color(0.03f,0.065f,0.075f,1));
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
