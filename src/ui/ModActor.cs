using NeoRune;
using UE.Engine;
using UE.CoreUObject;
using UE.UMG;
using UE.GameSettings;
using UE.SWSettings;
using UE.CommonUI;
using UE.Angelscript;
using UE.SlateCore;
using UE.SpicewoodUI;
using System.Collections.Generic;
namespace MCD2Graphics;

public class GraphicsSettingsSave : USaveGame {
 public int SchemaVersion;
 public int Revision;
 public int ReconstructionMode;
 public int RenderScaleBasisPoints;
 public bool NativeFallback;
 public int SRPreset;
 public int CustomScaleBasisPoints;
 public int LastCustomScaleBasisPoints;
 public int RenderContextReady;
 public int RenderContextSessionId;
 public int AppliedSourceRevision;
 public int AppliedSourceSessionId;
}
public class GraphicsRuntimeStateSave : USaveGame {
 public int SchemaVersion;public int RequestedRevision;public int SessionId;public int Phase;public int ErrorCode;
}
public class ReconstructionHeaderSetting : UGameSettingCollection {}
public class ReconstructionHeaderRow : UGameSettingListEntryBase {
 public UAS_SettingsEntry_Header? Skin;
 public override void OnInitialized(){
  Skin=UWidgetBlueprintLibrary.Create(this,Unreal.ClassAt<UAS_SettingsEntry_Header>("/SpicewoodSettings/Spicewood/UI/SettingsScreen/Editors/W_SettingsEntry_Header.W_SettingsEntry_Header_C"),UGameplayStatics.GetPlayerController(this,0)) as UAS_SettingsEntry_Header;
  if(Skin==null || Skin.WidgetTree==null || Skin.WidgetTree.RootWidget==null || WidgetTree==null){Log.Write("RECONSTRUCTION HEADER TREE FAILED");return;}
  WidgetTree.RootWidget=Skin.WidgetTree.RootWidget;bIsFocusable=false;Skin.bIsFocusable=false;
  WidgetTree.RootWidget.SetVisibility(ESlateVisibility.HitTestInvisible);Refresh();
 }
 public override void Construct(){Refresh();}
 public void Refresh(){if(Skin!=null && Skin.Text_SettingName!=null)Skin.Text_SettingName.SetText("RECONSTRUCTION");}
 protected override UWidget? GetPrimaryGamepadFocusWidget(){return null;}
}
public class FGHeaderSetting : UGameSettingCollection {}
public class FGHeaderRow : UGameSettingListEntryBase {
 public UAS_SettingsEntry_Header? Skin;
 public override void OnInitialized(){
  Skin=UWidgetBlueprintLibrary.Create(this,Unreal.ClassAt<UAS_SettingsEntry_Header>("/SpicewoodSettings/Spicewood/UI/SettingsScreen/Editors/W_SettingsEntry_Header.W_SettingsEntry_Header_C"),UGameplayStatics.GetPlayerController(this,0)) as UAS_SettingsEntry_Header;
  if(Skin==null || Skin.WidgetTree==null || Skin.WidgetTree.RootWidget==null || WidgetTree==null){Log.Write("FRAME GENERATION HEADER TREE FAILED");return;}
  WidgetTree.RootWidget=Skin.WidgetTree.RootWidget;bIsFocusable=false;Skin.bIsFocusable=false;
  WidgetTree.RootWidget.SetVisibility(ESlateVisibility.HitTestInvisible);Refresh();
 }
 public override void Construct(){Refresh();}
 public void Refresh(){if(Skin!=null && Skin.Text_SettingName!=null)Skin.Text_SettingName.SetText("FRAME GENERATION");}
 protected override UWidget? GetPrimaryGamepadFocusWidget(){return null;}
}
public class ReconstructionSetting : UGameSetting {
 public string OwnedDisplayName;
 public string OwnedHelpText;
}
public class PresetSetting : UGameSetting {public string OwnedDisplayName;public string OwnedHelpText;}
public class ScaleSetting : UGameSetting {public string OwnedDisplayName;public string OwnedHelpText;}
public class ReconstructionRow : UGameSettingListEntryBase {
 public UAS_SettingsEntry_Stepper? Skin;
 public ModActor? Manager;
 public override void OnInitialized() {
  UGameplayStatics.GetAllActorsOfClass(this,Unreal.ClassOf<ModActor>(),out List<AActor> actors);
  if(actors.Count>0)Manager=actors[0] as ModActor;
  Skin=UWidgetBlueprintLibrary.Create(this,Unreal.ClassAt<UAS_SettingsEntry_Stepper>("/SpicewoodSettings/Spicewood/UI/SettingsScreen/Editors/W_SettingsEntry_Stepper.W_SettingsEntry_Stepper_C"),UGameplayStatics.GetPlayerController(this,0)) as UAS_SettingsEntry_Stepper;
  if(Skin==null || Skin.WidgetTree==null || Skin.WidgetTree.RootWidget==null || WidgetTree==null) {Log.Write("ROW TREE FAILED");return;}
  WidgetTree.RootWidget=Skin.WidgetTree.RootWidget;bIsFocusable=true;
  Skin.WidgetTree.RootWidget.SetVisibility(ESlateVisibility.Visible);
  // Visual shell only: native hover/click callbacks expect a native DiscreteSetting.
  if(Skin.Button_Decrease!=null){Skin.Button_Decrease.SetVisibility(ESlateVisibility.HitTestInvisible);Skin.Button_Decrease.SetIsFocusable(false);}
  if(Skin.Button_Increase!=null){Skin.Button_Increase.SetVisibility(ESlateVisibility.HitTestInvisible);Skin.Button_Increase.SetIsFocusable(false);}
  if(Skin.Rotator_SettingValue!=null)Skin.Rotator_SettingValue.SetVisibility(ESlateVisibility.HitTestInvisible);
  if(Skin.Text_SettingName!=null)Skin.Text_SettingName.SetText("Upscaler");
  if(Skin.Rotator_SettingValue!=null){
   var choices=new List<FText>{"Native","NVIDIA DLSS"};
   Skin.Rotator_SettingValue.PopulateTextLabels(choices);
  }
  Refresh();Log.Write("RECONSTRUCTION ROW READY");
 }
 public override void Construct(){Refresh();}
 public void Refresh(){
  if(Skin!=null && Manager!=null && Skin.Rotator_SettingValue!=null){
   var choices=Manager.SharedControls()?new List<FText>{"Native","NVIDIA DLSS","AMD FSR"}:new List<FText>{"Native","NVIDIA DLSS"};
   Skin.Rotator_SettingValue.PopulateTextLabels(choices);Skin.Rotator_SettingValue.SetSelectedItem(Manager.SelectedProvider());
  }
 }
 void Change(int direction){if(Manager!=null){Manager.ChangeMode(direction);Refresh();Manager.ShowReconstructionHelp();SetUserFocus(UGameplayStatics.GetPlayerController(this,0));}}
 protected override UWidget? GetPrimaryGamepadFocusWidget(){return this;}
 public override FEventReply OnPreviewMouseButtonDown(FGeometry geo,FPointerEvent ev){if(Manager!=null)Manager.SelectOwned(Manager.Model);
  if(UKismetInputLibrary.PointerEvent_GetEffectingButton(ev).KeyName!="LeftMouseButton")return UWidgetBlueprintLibrary.Unhandled();
  var pos=UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(ev);
  if(Skin!=null && Skin.Button_Decrease!=null && USlateBlueprintLibrary.IsUnderLocation(Skin.Button_Decrease.GetCachedGeometry(),pos))Change(-1);
  else if(Skin!=null && Skin.Button_Increase!=null && USlateBlueprintLibrary.IsUnderLocation(Skin.Button_Increase.GetCachedGeometry(),pos))Change(1);
  else {SetUserFocus(UGameplayStatics.GetPlayerController(this,0));if(Manager!=null)Manager.ShowReconstructionHelp();}
  var focusReply=UWidgetBlueprintLibrary.Handled();focusReply=UWidgetBlueprintLibrary.SetUserFocus(ref focusReply,this,false);return UWidgetBlueprintLibrary.CaptureMouse(ref focusReply,this);
 }
 public override FEventReply OnMouseButtonDoubleClick(FGeometry geo,FPointerEvent ev){return OnPreviewMouseButtonDown(geo,ev);}
 public override FEventReply OnMouseButtonUp(FGeometry geo,FPointerEvent ev){if(UKismetInputLibrary.PointerEvent_GetEffectingButton(ev).KeyName!="LeftMouseButton")return UWidgetBlueprintLibrary.Unhandled();var focusReply=UWidgetBlueprintLibrary.Handled();return UWidgetBlueprintLibrary.ReleaseMouseCapture(ref focusReply);}
 public override FEventReply OnPreviewKeyDown(FGeometry geo,FKeyEvent ev){
  var k=UKismetInputLibrary.GetKey(ev).KeyName;
  if(k=="Left" || k=="Gamepad_DPad_Left"){Change(-1);return UWidgetBlueprintLibrary.Handled();}
  if(k=="Right" || k=="Gamepad_DPad_Right" || k=="Enter" || k=="SpaceBar" || k=="Gamepad_FaceButton_Bottom"){Change(1);return UWidgetBlueprintLibrary.Handled();}
  return UWidgetBlueprintLibrary.Unhandled();
 }
 public override void OnMouseEnter(FGeometry geo,FPointerEvent ev){if(Manager!=null)Manager.ShowReconstructionHelp();}
 public override FEventReply OnFocusReceived(FGeometry geo,FFocusEvent ev){if(Manager!=null)Manager.ShowReconstructionHelp();return UWidgetBlueprintLibrary.Handled();}
}
public class PresetRow : UGameSettingListEntryBase {
 public UAS_SettingsEntry_Stepper? Skin;
 public ModActor? Manager;
 public override void OnInitialized() {
  UGameplayStatics.GetAllActorsOfClass(this,Unreal.ClassOf<ModActor>(),out List<AActor> actors);
  if(actors.Count>0)Manager=actors[0] as ModActor;
  Skin=UWidgetBlueprintLibrary.Create(this,Unreal.ClassAt<UAS_SettingsEntry_Stepper>("/SpicewoodSettings/Spicewood/UI/SettingsScreen/Editors/W_SettingsEntry_Stepper.W_SettingsEntry_Stepper_C"),UGameplayStatics.GetPlayerController(this,0)) as UAS_SettingsEntry_Stepper;
  if(Skin==null || Skin.WidgetTree==null || Skin.WidgetTree.RootWidget==null || WidgetTree==null) {Log.Write("ROW TREE FAILED");return;}
  WidgetTree.RootWidget=Skin.WidgetTree.RootWidget;bIsFocusable=true;
  Skin.WidgetTree.RootWidget.SetVisibility(ESlateVisibility.Visible);
  // Visual shell only: native hover/click callbacks expect a native DiscreteSetting.
  if(Skin.Button_Decrease!=null){Skin.Button_Decrease.SetVisibility(ESlateVisibility.HitTestInvisible);Skin.Button_Decrease.SetIsFocusable(false);}
  if(Skin.Button_Increase!=null){Skin.Button_Increase.SetVisibility(ESlateVisibility.HitTestInvisible);Skin.Button_Increase.SetIsFocusable(false);}
  if(Skin.Rotator_SettingValue!=null)Skin.Rotator_SettingValue.SetVisibility(ESlateVisibility.HitTestInvisible);
  if(Skin.Text_SettingName!=null)Skin.Text_SettingName.SetText("DLSS Preset");
  if(Skin.Rotator_SettingValue!=null){
   var choices=new List<FText>{"DLAA","Quality","Balanced","Performance","Ultra Performance","Custom"};
   Skin.Rotator_SettingValue.PopulateTextLabels(choices);
  }
  Refresh();Log.Write("RECONSTRUCTION ROW READY");
 }
 public override void Construct(){Refresh();}
 public void Refresh(){
  if(Skin!=null && Manager!=null && Skin.Rotator_SettingValue!=null){
   if(Skin.Text_SettingName!=null)Skin.Text_SettingName.SetText(Manager.PresetTitle());
   Skin.Rotator_SettingValue.PopulateTextLabels(new List<FText>{Manager.FullResolutionName(),"Quality","Balanced","Performance","Ultra Performance","Custom"});
   Skin.Rotator_SettingValue.SetSelectedItem(Manager.SelectedQuality());
  }
 }
 void Change(int direction){if(Manager!=null){Manager.ChangePreset(direction);Refresh();Manager.ShowPresetHelp();SetUserFocus(UGameplayStatics.GetPlayerController(this,0));}}
 protected override UWidget? GetPrimaryGamepadFocusWidget(){return this;}
 public override FEventReply OnPreviewMouseButtonDown(FGeometry geo,FPointerEvent ev){if(Manager!=null)Manager.SelectOwned(Manager.PresetModel);
  if(UKismetInputLibrary.PointerEvent_GetEffectingButton(ev).KeyName!="LeftMouseButton")return UWidgetBlueprintLibrary.Unhandled();
  var pos=UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(ev);
  if(Skin!=null && Skin.Button_Decrease!=null && USlateBlueprintLibrary.IsUnderLocation(Skin.Button_Decrease.GetCachedGeometry(),pos))Change(-1);
  else if(Skin!=null && Skin.Button_Increase!=null && USlateBlueprintLibrary.IsUnderLocation(Skin.Button_Increase.GetCachedGeometry(),pos))Change(1);
  else {SetUserFocus(UGameplayStatics.GetPlayerController(this,0));if(Manager!=null)Manager.ShowPresetHelp();}
  var focusReply=UWidgetBlueprintLibrary.Handled();focusReply=UWidgetBlueprintLibrary.SetUserFocus(ref focusReply,this,false);return UWidgetBlueprintLibrary.CaptureMouse(ref focusReply,this);
 }
 public override FEventReply OnMouseButtonDoubleClick(FGeometry geo,FPointerEvent ev){return OnPreviewMouseButtonDown(geo,ev);}
 public override FEventReply OnMouseButtonUp(FGeometry geo,FPointerEvent ev){if(UKismetInputLibrary.PointerEvent_GetEffectingButton(ev).KeyName!="LeftMouseButton")return UWidgetBlueprintLibrary.Unhandled();var focusReply=UWidgetBlueprintLibrary.Handled();return UWidgetBlueprintLibrary.ReleaseMouseCapture(ref focusReply);}
 public override FEventReply OnPreviewKeyDown(FGeometry geo,FKeyEvent ev){
  var k=UKismetInputLibrary.GetKey(ev).KeyName;
  if(k=="Left" || k=="Gamepad_DPad_Left"){Change(-1);return UWidgetBlueprintLibrary.Handled();}
  if(k=="Right" || k=="Gamepad_DPad_Right" || k=="Enter" || k=="SpaceBar" || k=="Gamepad_FaceButton_Bottom"){Change(1);return UWidgetBlueprintLibrary.Handled();}
  return UWidgetBlueprintLibrary.Unhandled();
 }
 public override void OnMouseEnter(FGeometry geo,FPointerEvent ev){if(Manager!=null)Manager.ShowPresetHelp();}
 public override FEventReply OnFocusReceived(FGeometry geo,FFocusEvent ev){if(Manager!=null)Manager.ShowPresetHelp();return UWidgetBlueprintLibrary.Handled();}
}
public class ScaleRow : UGameSettingListEntryBase {
 public UAS_SettingsEntry_Scalar? Skin;public ModActor? Manager;bool dragging;
 public override void OnInitialized(){
  UGameplayStatics.GetAllActorsOfClass(this,Unreal.ClassOf<ModActor>(),out List<AActor> actors);if(actors.Count>0)Manager=actors[0] as ModActor;
  Skin=UWidgetBlueprintLibrary.Create(this,Unreal.ClassAt<UAS_SettingsEntry_Scalar>("/SpicewoodSettings/Spicewood/UI/SettingsScreen/Editors/W_SettingsEntry_Scalar.W_SettingsEntry_Scalar_C"),UGameplayStatics.GetPlayerController(this,0)) as UAS_SettingsEntry_Scalar;
  if(Skin==null || Skin.WidgetTree==null || Skin.WidgetTree.RootWidget==null || WidgetTree==null)return;WidgetTree.RootWidget=Skin.WidgetTree.RootWidget;bIsFocusable=true;
  Skin.WidgetTree.RootWidget.SetVisibility(ESlateVisibility.Visible);
  if(Skin.Text_SettingName!=null)Skin.Text_SettingName.SetText("Render Scale");
  if(Skin.Slider_SettingValue!=null){Skin.Slider_SettingValue.SetMinValue(0);Skin.Slider_SettingValue.SetMaxValue(1);Skin.Slider_SettingValue.SetStepSize(1f/99f);Skin.Slider_SettingValue.SetLocked(true);}Refresh();
 }
 public override void Construct(){Refresh();}
 public void Refresh(){if(Skin==null || Manager==null || Manager.Saved==null)return;int span=100-Manager.MinimumScalePercent;float value=span>0?((Manager.SelectedScale()==0?6700:Manager.SelectedScale())/100-Manager.MinimumScalePercent)/(float)span:1f;if(value<0)value=0;if(value>1)value=1;if(Skin.Slider_SettingValue!=null){Skin.Slider_SettingValue.SetStepSize(span>0?1f/span:1f);Skin.Slider_SettingValue.SetValue(value);}if(Skin.Slider_Bar!=null){var material=Skin.Slider_Bar.GetDynamicMaterial();if(material!=null)material.SetScalarParameterValue("Value",value);}if(Skin.Text_SettingValue!=null)Skin.Text_SettingValue.SetText(Manager.SelectedScale()==0?"Auto":(Manager.SelectedScale()/100)+"%");}
 void AtPointer(FPointerEvent ev){if(Manager==null || Skin==null || Skin.Slider_SettingValue==null)return;var geo=Skin.Slider_SettingValue.GetCachedGeometry();var size=USlateBlueprintLibrary.GetLocalSize(geo);if(size.X<=0)return;var pos=USlateBlueprintLibrary.AbsoluteToLocal(geo,UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(ev));int percent=Manager.MinimumScalePercent+(int)(pos.X/size.X*(100-Manager.MinimumScalePercent)+0.5);if(percent<Manager.MinimumScalePercent)percent=Manager.MinimumScalePercent;if(percent>100)percent=100;Manager.ChangeScale(percent*100);Refresh();Manager.ShowScaleHelp();}
 protected override UWidget? GetPrimaryGamepadFocusWidget(){return this;}
 public override FEventReply OnPreviewMouseButtonDown(FGeometry geo,FPointerEvent ev){if(Manager!=null)Manager.SelectOwned(Manager.ScaleModel);if(UKismetInputLibrary.PointerEvent_GetEffectingButton(ev).KeyName!="LeftMouseButton")return UWidgetBlueprintLibrary.Unhandled();SetUserFocus(UGameplayStatics.GetPlayerController(this,0));if(Manager!=null)Manager.ShowScaleHelp();if(Skin!=null && Skin.Slider_SettingValue!=null && USlateBlueprintLibrary.IsUnderLocation(Skin.Slider_SettingValue.GetCachedGeometry(),UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(ev))){dragging=true;AtPointer(ev);var reply=UWidgetBlueprintLibrary.Handled();reply=UWidgetBlueprintLibrary.SetUserFocus(ref reply,this,false);return UWidgetBlueprintLibrary.CaptureMouse(ref reply,this);}var focusReply=UWidgetBlueprintLibrary.Handled();return UWidgetBlueprintLibrary.SetUserFocus(ref focusReply,this,false);}
 public override FEventReply OnMouseMove(FGeometry geo,FPointerEvent ev){if(!dragging)return UWidgetBlueprintLibrary.Unhandled();AtPointer(ev);return UWidgetBlueprintLibrary.Handled();}
 public override FEventReply OnMouseButtonDoubleClick(FGeometry geo,FPointerEvent ev){return OnPreviewMouseButtonDown(geo,ev);}
 public override FEventReply OnMouseButtonUp(FGeometry geo,FPointerEvent ev){if(!dragging)return UWidgetBlueprintLibrary.Unhandled();dragging=false;AtPointer(ev);var reply=UWidgetBlueprintLibrary.Handled();return UWidgetBlueprintLibrary.ReleaseMouseCapture(ref reply);}
 public override FEventReply OnPreviewKeyDown(FGeometry geo,FKeyEvent ev){var k=UKismetInputLibrary.GetKey(ev).KeyName;int d=0;if(k=="Left" || k=="Gamepad_DPad_Left")d=-1;if(k=="Right" || k=="Gamepad_DPad_Right")d=1;if(d==0 || Manager==null || Manager.Saved==null)return UWidgetBlueprintLibrary.Unhandled();Manager.ChangeScale((Manager.SelectedScale()==0?6700:Manager.SelectedScale())+d*100);Refresh();Manager.ShowScaleHelp();return UWidgetBlueprintLibrary.Handled();}
 public override void OnMouseEnter(FGeometry geo,FPointerEvent ev){if(Manager!=null)Manager.ShowScaleHelp();}
 public override FEventReply OnFocusReceived(FGeometry geo,FFocusEvent ev){if(Manager!=null)Manager.ShowScaleHelp();return UWidgetBlueprintLibrary.Handled();}
}

// Separate display/latency intent leaves the released SR schema unchanged.
public class FGSettingsSave : USaveGame {public int SchemaVersion;public int Revision;public int Mode;public int ContextReady;public int SessionId;public int FGProvider;public int SettingsRevision;}
public class FGRuntimeSave : USaveGame {public int SchemaVersion;public int Revision;public int SessionId;public int Available;public int Active;public int Phase;public int RestartRequired;public int AvailableProviders;public int StartupOwner;}
public class DisplaySettingsSave : USaveGame {
 public int SchemaVersion;public int Revision;public int HDROutput;public int PeakNits;public int PaperWhiteNits;public int UINits;public int ReflexMode;
}
public class DisplayRuntimeSave : USaveGame {
 public int SchemaVersion;public int Revision;public int SessionId;public int ReflexAvailable;public int ReflexMode;public int ReflexFault;public int HDRRevision;public int HDRRestartRequired;
 public int AmdAntiLagAvailable;public int AmdAntiLagMode;public int AmdAntiLagFault;public int AmdAntiLagRevision;
 public int AmdAntiLagFgCompatible;
}
public class DisplaySetting : UGameSetting {public int ControlId;}
public class DisplayRow : UGameSettingListEntryBase {
 public UAS_SettingsEntry_Stepper? Skin;public ModActor? Manager;int configured;
 public override void OnInitialized(){
  configured=-1;UGameplayStatics.GetAllActorsOfClass(this,Unreal.ClassOf<ModActor>(),out List<AActor> actors);if(actors.Count>0)Manager=actors[0] as ModActor;
  Skin=UWidgetBlueprintLibrary.Create(this,Unreal.ClassAt<UAS_SettingsEntry_Stepper>("/SpicewoodSettings/Spicewood/UI/SettingsScreen/Editors/W_SettingsEntry_Stepper.W_SettingsEntry_Stepper_C"),UGameplayStatics.GetPlayerController(this,0)) as UAS_SettingsEntry_Stepper;
  if(Skin==null || Skin.WidgetTree==null || Skin.WidgetTree.RootWidget==null || WidgetTree==null)return;
  WidgetTree.RootWidget=Skin.WidgetTree.RootWidget;bIsFocusable=true;Skin.WidgetTree.RootWidget.SetVisibility(ESlateVisibility.Visible);
  if(Skin.Button_Decrease!=null){Skin.Button_Decrease.SetVisibility(ESlateVisibility.HitTestInvisible);Skin.Button_Decrease.SetIsFocusable(false);}
  if(Skin.Button_Increase!=null){Skin.Button_Increase.SetVisibility(ESlateVisibility.HitTestInvisible);Skin.Button_Increase.SetIsFocusable(false);}
  if(Skin.Rotator_SettingValue!=null)Skin.Rotator_SettingValue.SetVisibility(ESlateVisibility.HitTestInvisible);Refresh();
 }
 public override void Construct(){Refresh();}
 public void Refresh(){var model=Setting as DisplaySetting;if(model==null || Skin==null || Manager==null || Manager.DisplaySaved==null)return;int id=model.ControlId;
  if(configured!=id){configured=id;if(Skin.Text_SettingName!=null)Skin.Text_SettingName.SetText(Manager.DisplayLabel(id));
   var choices=new List<FText>();if(id==0 || id==5 || id==6){choices.Add("Off");choices.Add("On");}else if(id==7){choices.Add("NVIDIA DLSS");choices.Add("AMD FSR");}else if(id==4){choices.Add("Off");choices.Add("On");choices.Add("On + Boost");}else {int max=id==1?10000:500;int step=id==1?10:1;for(int n=id==1?100:48;n<=max;n+=step)choices.Add(n+" nits");}
   if(Skin.Rotator_SettingValue!=null)Skin.Rotator_SettingValue.PopulateTextLabels(choices);
  }
  int value=Manager.DisplayValue(id);int index=id==1?(value-100)/10:(id==2 || id==3?value-48:value);
  if(Skin.Rotator_SettingValue!=null)Skin.Rotator_SettingValue.SetSelectedItem(index);
  SetIsEnabled(id==6?Manager.AmdLatencyAvailable():id==7?Manager.SharedFgAvailable():id==5?Manager.SelectedFgAvailable():id==0 || (id==4?(Manager.DisplayRuntime!=null && Manager.DisplayRuntime.ReflexAvailable==1 && Manager.DisplayRuntime.ReflexFault==0):Manager.DisplaySaved.HDROutput==1));
 }
 void Change(int direction){var model=Setting as DisplaySetting;if(model==null || Manager==null)return;Manager.ChangeDisplay(model.ControlId,direction);Refresh();Manager.ShowDisplayHelp(model.ControlId);SetUserFocus(UGameplayStatics.GetPlayerController(this,0));}
 protected override UWidget? GetPrimaryGamepadFocusWidget(){return this;}
 public override FEventReply OnPreviewMouseButtonDown(FGeometry geo,FPointerEvent ev){var model=Setting as DisplaySetting;if(model!=null && Manager!=null)Manager.SelectOwned(model);
  if(UKismetInputLibrary.PointerEvent_GetEffectingButton(ev).KeyName!="LeftMouseButton")return UWidgetBlueprintLibrary.Unhandled();var pos=UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(ev);
  if(Skin!=null && Skin.Button_Decrease!=null && USlateBlueprintLibrary.IsUnderLocation(Skin.Button_Decrease.GetCachedGeometry(),pos))Change(-1);
  else if(Skin!=null && Skin.Button_Increase!=null && USlateBlueprintLibrary.IsUnderLocation(Skin.Button_Increase.GetCachedGeometry(),pos))Change(1);
  else {SetUserFocus(UGameplayStatics.GetPlayerController(this,0));if(Manager!=null && model!=null)Manager.ShowDisplayHelp(model.ControlId);}
  var reply=UWidgetBlueprintLibrary.Handled();reply=UWidgetBlueprintLibrary.SetUserFocus(ref reply,this,false);return UWidgetBlueprintLibrary.CaptureMouse(ref reply,this);
 }
 public override FEventReply OnMouseButtonUp(FGeometry geo,FPointerEvent ev){if(UKismetInputLibrary.PointerEvent_GetEffectingButton(ev).KeyName!="LeftMouseButton")return UWidgetBlueprintLibrary.Unhandled();var reply=UWidgetBlueprintLibrary.Handled();return UWidgetBlueprintLibrary.ReleaseMouseCapture(ref reply);}
 public override FEventReply OnMouseButtonDoubleClick(FGeometry geo,FPointerEvent ev){return OnPreviewMouseButtonDown(geo,ev);}
 public override FEventReply OnPreviewKeyDown(FGeometry geo,FKeyEvent ev){var k=UKismetInputLibrary.GetKey(ev).KeyName;if(k=="Left" || k=="Gamepad_DPad_Left"){Change(-1);return UWidgetBlueprintLibrary.Handled();}if(k=="Right" || k=="Gamepad_DPad_Right" || k=="Enter" || k=="SpaceBar" || k=="Gamepad_FaceButton_Bottom"){Change(1);return UWidgetBlueprintLibrary.Handled();}return UWidgetBlueprintLibrary.Unhandled();}
 public override void OnMouseEnter(FGeometry geo,FPointerEvent ev){var m=Setting as DisplaySetting;if(Manager!=null && m!=null)Manager.ShowDisplayHelp(m.ControlId);}
 public override FEventReply OnFocusReceived(FGeometry geo,FFocusEvent ev){var m=Setting as DisplaySetting;if(Manager!=null && m!=null)Manager.ShowDisplayHelp(m.ControlId);return UWidgetBlueprintLibrary.Handled();}
}

public class ModActor : AActor {
 public bool ProviderOwned;
 public int ProviderUiFamily=1;
 int providerView=-1;int providerQualityView=-1;int providerScaleView=-1;int providerPhaseView=-1;
 public bool SharedControls(){return ProviderOwned && ProviderMenu!=null && ProviderMenu.Ready;}
 public bool AmdLatencyAvailable(){
  return SharedControls() && ProviderMenu!=null && ProviderMenu.Authority!=null && DisplayRuntime!=null &&
   ProviderRuntimeClient.AmdLatencyAvailable(true,ProviderMenu.Authority.W5,DisplayRuntime.SchemaVersion,DisplayRuntime.SessionId,DisplayRuntime.AmdAntiLagAvailable,DisplayRuntime.AmdAntiLagFault,ProviderMenu.Value(19)!=0,ProviderMenu.Value(20),DisplayRuntime.AmdAntiLagFgCompatible);
 }
 public int SelectedProvider(){return SharedControls() && ProviderMenu!=null?ProviderMenu.Value(8):(Saved!=null && Saved.ReconstructionMode!=0?1:0);}
 public int SelectedFamily(){int provider=SelectedProvider();if(provider==1 || provider==2)ProviderUiFamily=provider;return ProviderUiFamily;}
 public int SelectedQuality(){if(SharedControls() && ProviderMenu!=null)return ProviderMenu.Value(ProviderMenu.PreferenceWord(SelectedFamily()));return Saved==null?1:Saved.SRPreset==5?0:Saved.SRPreset==4?5:Saved.SRPreset+1;}
 public int SelectedScale(){
  if(SharedControls() && ProviderMenu!=null){int family=SelectedFamily();int quality=SelectedQuality();
   if(quality==0)return 10000;if(quality==5)return ProviderMenu.Value(ProviderMenu.PreferenceWord(family)+1);
   if(family==1)return quality==1?6700:quality==2?5800:quality==3?5000:3300;
   if(ProviderRuntime!=null && ProviderRuntime.State!=null){var state=ProviderRuntime.State;
    if(state.W18==family && state.W19==quality && state.W10==5)return ((state.W16+500000)/1000000)*100;}
   return 0; // SDK-derived named ratio is pending, never borrowed from NVIDIA.
  }
  return Saved==null?6700:Saved.CustomScaleBasisPoints;
 }
 public string PresetTitle(){return SharedControls() && SelectedFamily()==2?"FSR Preset":"DLSS Preset";}
 public string FullResolutionName(){return SharedControls() && SelectedFamily()==2?"Native AA":"DLAA";}
 public void ProjectCommittedDisplay(){
  if(!SharedControls() || ProviderMenu==null || ProviderMenu.Authority==null || DisplaySaved==null)return;
  var a=ProviderMenu.Authority;int reflex=(a.W35==0 || a.W35==1)?a.W36:0;
  bool changed=DisplaySaved.Revision!=a.W19 || DisplaySaved.HDROutput!=a.W37 || DisplaySaved.PeakNits!=a.W38 || DisplaySaved.PaperWhiteNits!=a.W39 || DisplaySaved.UINits!=a.W40 || DisplaySaved.ReflexMode!=reflex;
  if(changed){DisplaySaved.Revision=a.W19;DisplaySaved.HDROutput=a.W37;DisplaySaved.PeakNits=a.W38;DisplaySaved.PaperWhiteNits=a.W39;DisplaySaved.UINits=a.W40;DisplaySaved.ReflexMode=reflex;PersistDisplay();}
  if(FGSaved!=null && (FGSaved.SettingsRevision!=a.W19 || FGSaved.Mode!=a.W31 || FGSaved.FGProvider!=a.W32)){FGSaved.SettingsRevision=a.W19;FGSaved.Revision++;FGSaved.Mode=a.W31;FGSaved.FGProvider=a.W32;PersistFG();}
 }
 public void ProjectCommittedNvidia(){
  if(!SharedControls() || ProviderMenu==null || ProviderMenu.Authority==null || Saved==null)return;
  var a=ProviderMenu.Authority;int quality=a.W21;int custom=quality==0?10000:quality==1?6700:quality==2?5800:quality==3?5000:quality==4?3300:a.W22;
  int mode=a.W20==1?(quality==0?1:2):0;int preset=quality==0?5:quality==5?4:quality-1;
  int ready=ProviderRuntime!=null && ProviderRuntime.Context!=null?ProviderRuntime.Context.W10:0;
  bool changed=Saved.SchemaVersion!=4 || Saved.Revision!=a.W19 || Saved.SRPreset!=preset || Saved.CustomScaleBasisPoints!=custom || Saved.LastCustomScaleBasisPoints!=a.W23 || Saved.ReconstructionMode!=mode || Saved.NativeFallback!=(a.W30==1) || Saved.RenderContextReady!=ready;
  if(!changed)return;
  Saved.SchemaVersion=4;Saved.Revision=a.W19;Saved.SRPreset=preset;
  Saved.CustomScaleBasisPoints=custom;Saved.LastCustomScaleBasisPoints=a.W23;
  Saved.ReconstructionMode=mode;Saved.NativeFallback=a.W30==1;
  Saved.RenderContextReady=ready;
  UpdateScale();Persist(); // Compatibility projection/source ACK only; input callbacks write the shared request.
 }

 public ProviderMenuClient? ProviderMenu;
 public ProviderRuntimeClient? ProviderRuntime;
 public void PollProviders(){
  if(ProviderMenu==null || ProviderRuntime==null)return;
  ProviderMenu.Poll();
  var level=UGameplayStatics.GetCurrentLevelName(this,true);
  bool ready=level!="" && level!="Menu_Spicewood" && World.Player(this)!=null;
  float units=0;var worlds=World.FindAll(this,Unreal.ClassOf<AWorldSettings>());
  if(worlds.Count>0){var settings=worlds[0] as AWorldSettings;if(settings!=null)units=settings.WorldToMeters;}
  float percentage=UKismetSystemLibrary.GetConsoleVariableFloatValue("r.ScreenPercentage");
  if(!(units>0 && units<=1000000)){units=0;ready=false;}
  if(!(percentage>=1 && percentage<=100)){percentage=0;ready=false;}
  ProviderRuntime.Poll(ProviderMenu,level,ready,(int)(units*1000),(int)(percentage*1000000));
  if(ProviderMenu.Ready && ProviderRuntime.Enabled)ProviderOwned=true;
  if(SharedControls()){
   ProjectCommittedDisplay();
   int provider=SelectedProvider(),quality=SelectedQuality(),scale=SelectedScale();int phase=ProviderRuntime.State==null?0:ProviderRuntime.State.W10;
   if(provider!=providerView || quality!=providerQualityView || scale!=providerScaleView || phase!=providerPhaseView){
    providerView=provider;providerQualityView=quality;providerScaleView=scale;providerPhaseView=phase;ProjectCommittedNvidia();SyncLists();RefreshRows();ShowHelp();}
  }
  if(ProviderRuntime.NeedsSourceCommand()){
   var player=UGameplayStatics.GetPlayerController(this,0);if(player==null)return;
   int target=ProviderRuntime.SourceTarget();string number=""+(target/1000000)+".";
   int fraction=target%1000000;for(int divisor=100000;divisor>=1;divisor/=10)number+=""+((fraction/divisor)%10);
   UKismetSystemLibrary.ExecuteConsoleCommand(this,"r.ScreenPercentage "+number,player);
   float observed=UKismetSystemLibrary.GetConsoleVariableFloatValue("r.ScreenPercentage");
   if(observed>=1 && observed<=100)ProviderRuntime.SourceObserved((int)(observed*1000000));
  }
 }
 // Keep our recommendation in the game's native footer, separate from status.
 public List<UGameSettingDetailView> HelpFooterPanels=new List<UGameSettingDetailView>();
 public List<UGameSettingDetailExtension_DefaultValue> HelpFooters=new List<UGameSettingDetailExtension_DefaultValue>();
 public FGSettingsSave? FGSaved;public FGRuntimeSave? FGRuntime;public FGHeaderSetting? FGHeaderModel;
 bool foliageVelocityOwned;bool foliageVelocityBlocked;int foliageVelocityOriginal;int foliageRestoreAttempts;
 public DisplaySettingsSave? DisplaySaved;public DisplayRuntimeSave? DisplayRuntime;public List<DisplaySetting> DisplayModels=new List<DisplaySetting>();bool amdMenuVisible;int displayAppliedRevision;bool displayHDRSupported;int displayHelp=-1;
 public GraphicsSettingsSave? Saved;
 public GraphicsRuntimeStateSave? Runtime;
 public int MinimumScalePercent=17;int outputWidth;int outputHeight;int sourceApplyTicks;int lastAppliedSourceRevision;int lastAppliedSourceSession;int lastFallbackRevision;int lastFallbackSession;
 public ReconstructionHeaderSetting? HeaderModel;
 public ReconstructionSetting? Model;public PresetSetting? PresetModel;public ScaleSetting? ScaleModel;public int HelpKind;public bool OwnHelpVisible;public UObject? PendingFocus;
 public List<USpicewoodGameSettingPanel> Panels=new List<USpicewoodGameSettingPanel>();
 public ReconstructionRow? Probe;
 public List<UCommonActivatableWidgetContainerBase> Layers=new List<UCommonActivatableWidgetContainerBase>();
 public List<UCommonActivatableWidget?> ActiveLayers=new List<UCommonActivatableWidget?>();
 public List<UCommonButtonBase> ResetButtons=new List<UCommonButtonBase>();
 public List<UCommonActivatableWidget> LifecycleWidgets=new List<UCommonActivatableWidget>();
 int discoveryAttempts;
 bool syncing;
 protected override void ReceiveBeginPlay(){
  Log.Write("RENDER CONTEXT LEVEL="+UGameplayStatics.GetCurrentLevelName(this,true));
  Saved=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsSettings",0) as GraphicsSettingsSave;
  if(Saved==null){
   Saved=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<GraphicsSettingsSave>()) as GraphicsSettingsSave;
   if(Saved!=null){Saved.SchemaVersion=4;Saved.LastCustomScaleBasisPoints=7700;Saved.CustomScaleBasisPoints=6700;Saved.SRPreset=0;Saved.Revision=1;Saved.ReconstructionMode=0;Saved.RenderScaleBasisPoints=10000;Saved.NativeFallback=true;Persist();}
  }
  if(Saved!=null && (Saved.SchemaVersion==1 || Saved.SchemaVersion==2 || Saved.SchemaVersion==3)){
   if(Saved.SchemaVersion==1){Saved.SRPreset=0;Saved.CustomScaleBasisPoints=6700;}
   // Older full-resolution custom requests used the separate DLAA mode.
   if(Saved.ReconstructionMode==2 && Saved.SRPreset==4 && Saved.CustomScaleBasisPoints>=10000)Saved.ReconstructionMode=1;
   if(Saved.ReconstructionMode==1)Saved.SRPreset=5;
   if(Saved.SRPreset!=4)Saved.CustomScaleBasisPoints=PresetSliderScale(Saved.SRPreset);
   else {if(Saved.CustomScaleBasisPoints<100)Saved.CustomScaleBasisPoints=100;if(Saved.CustomScaleBasisPoints>9900)Saved.CustomScaleBasisPoints=9900;Saved.SRPreset=PresetAtScale(Saved.CustomScaleBasisPoints);}
   Saved.LastCustomScaleBasisPoints=Saved.SRPreset==4?Saved.CustomScaleBasisPoints:7700;
   Saved.SchemaVersion=4;UpdateScale();Saved.Revision++;Persist();
  }
  if(Saved!=null && Saved.SchemaVersion==4){string level=UGameplayStatics.GetCurrentLevelName(this,true);Saved.RenderContextReady=level!="" && level!="Menu_Spicewood"?1:0;Saved.RenderContextSessionId=0;Saved.Revision++;Persist();}
  if(Saved!=null)Log.Write("SETTINGS version="+Saved.SchemaVersion+" revision="+Saved.Revision+" mode="+Saved.ReconstructionMode);
  Runtime=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsRuntime",0) as GraphicsRuntimeStateSave;
  if(Runtime==null){Runtime=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<GraphicsRuntimeStateSave>()) as GraphicsRuntimeStateSave;if(Runtime!=null){Runtime.SchemaVersion=1;UGameplayStatics.SaveGameToSlot(Runtime,"MCD2GraphicsRuntime",0);}}
  InitializeFG();Timer.Start(this,"PollFG",0.5f,true);InitializeDisplay();Timer.Start(this,"PollDisplay",0.5f,true);
  Timer.Start(this,"PollRenderer",0.25f,true);
  ProviderMenu=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderMenuClient>()) as ProviderMenuClient;
  ProviderRuntime=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderRuntimeClient>()) as ProviderRuntimeClient;
  if(ProviderMenu!=null && ProviderRuntime!=null){ProviderMenu.Start();ProviderRuntime.Start();Timer.Start(this,"PollProviders",0.25f,true);}
  Discover();if(LifecycleWidgets.Count==0)Timer.Start(this,"Discover",0.1f,true);
 }
 public void PollRenderer(){
  RefreshResolutionBounds();
  if(ProviderOwned){
   if(!SharedControls()){
    // Lost authority can finish an already admitted Native rollback only.
    // Never use the compatibility save to authorize reduced resolution.
    var fallback=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsRuntime",0) as GraphicsRuntimeStateSave;
    if(Saved!=null && fallback!=null && ProviderRuntimeClient.CanRestoreNvidiaSource(Saved.Revision,Saved.ReconstructionMode,Saved.RenderContextSessionId,fallback.SchemaVersion,fallback.RequestedRevision,fallback.SessionId,fallback.Phase) && (lastFallbackRevision!=Saved.Revision || lastFallbackSession!=fallback.SessionId)){
     var player=UGameplayStatics.GetPlayerController(this,0);if(player==null)return;
     UKismetSystemLibrary.ExecuteConsoleCommand(this,"r.ScreenPercentage 100",player);
     lastFallbackRevision=Saved.Revision;lastFallbackSession=fallback.SessionId;
    }
    return;
   }
   ProjectCommittedNvidia();
   if(ProviderMenu!=null && ProviderMenu.Authority!=null && ProviderMenu.Authority.W20==2)return;
  }
  RefreshResolutionBounds();
  if(Saved==null || Saved.SchemaVersion!=4 || Saved.Revision<1 || Saved.ReconstructionMode<0 || Saved.ReconstructionMode>2)return;
  var runtime=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsRuntime",0) as GraphicsRuntimeStateSave;
  if(runtime==null || runtime.SchemaVersion!=1 || runtime.RequestedRevision!=Saved.Revision || runtime.SessionId<1)return;bool statusChanged=Runtime==null || Runtime.Phase!=runtime.Phase || Runtime.RequestedRevision!=runtime.RequestedRevision || Runtime.SessionId!=runtime.SessionId;Runtime=runtime;if(statusChanged)ShowHelp();
  // A persisted gameplay flag from another process may never activate menu SR.
  if(Saved.RenderContextSessionId!=runtime.SessionId){Saved.RenderContextSessionId=runtime.SessionId;if(!ProviderOwned)Saved.Revision++;Persist();return;}
  if(runtime.Phase==1){
   bool first=lastAppliedSourceRevision!=Saved.Revision || lastAppliedSourceSession!=runtime.SessionId;if(!first){sourceApplyTicks++;if(sourceApplyTicks<4)return;}sourceApplyTicks=0;
   if(UGameplayStatics.GetPlayerController(this,0)==null)return;
   // This timer is a game-thread applier; input callbacks only save intent.
   // The renderer publishes Phase 1 only after restoring native history and retiring its feature.
   if(Saved.ReconstructionMode==2 && Saved.RenderContextReady==1){string scale="66.666666667";if(Saved.SRPreset==1)scale="58";if(Saved.SRPreset==2)scale="50";if(Saved.SRPreset==3)scale="33.333333333";if(Saved.SRPreset==4)scale=""+(Saved.CustomScaleBasisPoints/100);UKismetSystemLibrary.ExecuteConsoleCommand(this,"r.ScreenPercentage "+scale,UGameplayStatics.GetPlayerController(this,0));}
   else UKismetSystemLibrary.ExecuteConsoleCommand(this,"r.ScreenPercentage 100",UGameplayStatics.GetPlayerController(this,0));
   lastAppliedSourceRevision=Saved.Revision;lastAppliedSourceSession=runtime.SessionId;Saved.AppliedSourceRevision=Saved.Revision;Saved.AppliedSourceSessionId=runtime.SessionId;if(first){Persist();Log.Write("SOURCE SCALE ACK revision="+Saved.Revision+" session="+runtime.SessionId);}ShowHelp();
  }
  if(runtime.Phase==3 && (lastFallbackRevision!=Saved.Revision || lastFallbackSession!=runtime.SessionId)){
   UKismetSystemLibrary.ExecuteConsoleCommand(this,"r.ScreenPercentage 100",UGameplayStatics.GetPlayerController(this,0));lastFallbackRevision=Saved.Revision;lastFallbackSession=runtime.SessionId;Log.Write("NATIVE FALLBACK SOURCE RESTORED");ShowHelp();
  }
 }
 public void DiscoverLayers(UWidget? widget,int depth){
  if(widget==null || depth>12)return;
  var layer=widget as UCommonActivatableWidgetContainerBase;
  if(layer!=null && !Layers.Contains(layer)){Layers.Add(layer);ActiveLayers.Add(layer.DisplayedWidget);Log.Write("LAYER EDGE WATCH BOUND="+UKismetSystemLibrary.GetObjectName(layer));}
  var nested=widget as UUserWidget;if(nested!=null && nested.WidgetTree!=null)DiscoverLayers(nested.WidgetTree.RootWidget,depth+1);
  var panel=widget as UPanelWidget;if(panel!=null){for(int i=0;i<panel.GetChildrenCount() && i<64;i++)DiscoverLayers(panel.GetChildAt(i),depth+1);}
 }
 public override void ReceiveTick(float delta){
  bool changed=false;for(int i=0;i<Layers.Count;i++){var active=Layers[i].DisplayedWidget;if(active!=ActiveLayers[i]){ActiveLayers[i]=active;changed=true;}}
  if(changed){Log.Write("UI LAYER CHANGED");LifecycleChanged();}
 }
 public void LifecycleChanged(){Timer.Start(this,"Discover",0.1f,false);}
 public void Discover(){
  discoveryAttempts++;
  var layout=USpicewoodPrimaryGameLayout.GetRootLayout(UGameplayStatics.GetGameInstance(this));
  if(layout!=null && layout.WidgetTree!=null)DiscoverLayers(layout.WidgetTree.RootWidget,0);
  UWidgetBlueprintLibrary.GetAllWidgetsOfClass(this,out List<UUserWidget> lifecycle,Unreal.ClassOf<UCommonActivatableWidget>(),false);
  foreach(var w in lifecycle){var active=w as UCommonActivatableWidget;if(active==null || LifecycleWidgets.Contains(active))continue;LifecycleWidgets.Add(active);active.BP_OnWidgetActivated+=LifecycleChanged;active.BP_OnWidgetDeactivated+=LifecycleChanged;Log.Write("LIFECYCLE BOUND="+UKismetSystemLibrary.GetObjectName(active));}
  UWidgetBlueprintLibrary.GetAllWidgetsOfClass(this,out List<UUserWidget> found,Unreal.ClassOf<USpicewoodGameSettingPanel>(),false);
  foreach(var w in found){var p=w as USpicewoodGameSettingPanel;if(p==null || p.ListView_Settings==null || p.Registry==null || Panels.Contains(p))continue;
   Panels.Add(p);p.ListView_Settings.BP_OnEntriesGenerated+=EntriesGenerated;p.ListView_Settings.BP_OnEntryReleased+=EntryReleased;p.ListView_Settings.BP_OnItemSelectionChanged+=SelectionChanged;p.ListView_Settings.BP_OnItemIsHoveredChanged+=HoverChanged;Log.Write("LIST EVENTS BOUND");
  }
  BindResetConfirmations();
  if(LifecycleWidgets.Count>0){Timer.Stop(this,"Discover");SyncLists();Log.Write("BOOTSTRAP STOPPED; ACTIVATION EVENTS attempts="+discoveryAttempts);}
  else if(discoveryAttempts>=600){Timer.Stop(this,"Discover");Log.Write("BOOTSTRAP EXPIRED; NO PERMANENT POLL");}
 }
 public void EntryReleased(UUserWidget? widget){Timer.Start(this,"SyncLists",0.01f,false);Log.Write("ENTRY RELEASED; DEFERRED SYNC");}
 public bool GraphicsPageOpen(){foreach(var p in Panels){if(p.ListView_Settings!=null && Model!=null && p.ListView_Settings.GetListItems().Contains(Model) && p.IsVisible())return true;}return false;}
 public void BindResetConfirmations(){
  foreach(var b in ResetButtons)b.OnButtonBaseClicked-=ResetConfirmed;ResetButtons.Clear();
  if(!GraphicsPageOpen())return;
  UWidgetBlueprintLibrary.GetAllWidgetsOfClass(this,out List<UUserWidget> frames,Unreal.ClassOf<UAS_DialogFrame>(),false);
  foreach(var w in frames){var f=w as UAS_DialogFrame;if(f==null || f.BodyTextBlock==null || f.DialogButtons==null || f.DialogButtons.ConfirmButton==null || !f.IsVisible())continue;
   var text=UKismetTextLibrary.Conv_TextToString(f.BodyTextBlock.GetText());
   if(!text.Contains("reset these settings to default") || !text.Contains("Settings from other categories"))continue;
   var button=f.DialogButtons.ConfirmButton;if(ResetButtons.Contains(button))continue;ResetButtons.Add(button);button.OnButtonBaseClicked+=ResetConfirmed;Log.Write("NATIVE GRAPHICS RESET CONFIRMATION BOUND");
  }
 }
 public void ResetConfirmed(UCommonButtonBase? button){if(button==null || !ResetButtons.Contains(button) || !GraphicsPageOpen())return;ResetOwned();Timer.Start(this,"SyncLists",0.01f,false);Log.Write("NATIVE RESET CONFIRMED");}
 public void EntriesGenerated(int count){Log.Write("LIST GENERATED count="+count);SyncLists();}
 public void SelectionChanged(UObject? item,bool selected){if(selected)Timer.Start(this,"SyncLists",0.01f,false);if(selected && item is DisplaySetting){var ds=item as DisplaySetting;if(ds!=null)ShowDisplayHelp(ds.ControlId);}else if(selected && item==Model)ShowReconstructionHelp();else if(selected && item==PresetModel)ShowPresetHelp();else if(selected && item==ScaleModel)ShowScaleHelp();else if(selected)RestoreExtensions();}
 public void HoverChanged(UObject? item,bool hovered){if(hovered && item is DisplaySetting){var ds=item as DisplaySetting;if(ds!=null)ShowDisplayHelp(ds.ControlId);}else if(hovered && item==Model)ShowReconstructionHelp();else if(hovered && item==PresetModel)ShowPresetHelp();else if(hovered && item==ScaleModel)ShowScaleHelp();else if(hovered)RestoreExtensions();}
 public void RestoreExtensions(){displayHelp=-1;OwnHelpVisible=false;foreach(var footer in HelpFooters)if(UKismetSystemLibrary.IsValid(footer))footer.RemoveFromParent();foreach(var p in Panels)if(p.Details_Settings!=null && p.Details_Settings.Box_DetailsExtension!=null)p.Details_Settings.Box_DetailsExtension.SetVisibility(ESlateVisibility.SelfHitTestInvisible);}
 public string UpscalerHelpBody(){if(SharedControls())return "Choose how the image is rendered.\n\n• Native — game anti-aliasing\n• NVIDIA DLSS — RTX reconstruction\n• AMD FSR — reconstruction for supported GPUs";return "Choose how the image is rendered.\n\n• Native\nUses the game's anti-aliasing.\n\n• NVIDIA DLSS\nImproves performance or uses DLAA at full resolution. Requires an RTX GPU.";}
 public string PresetHelpBody(){if(SharedControls() && SelectedFamily()==2)return "Balance image quality and performance.\n\n• Native AA — full resolution\n• Quality / Balanced — more detail\n• Performance / Ultra Performance — more speed\n• Custom — your render scale";return "Balance image quality and performance.\n\n• DLAA — native resolution\n• Quality — best upscaled detail\n• Balanced — more performance\n• Performance — higher frame rates\n• Ultra Performance — lowest render resolution\n• Custom — your chosen scale";}
 public string ScaleHelpBody(){if(SharedControls() && SelectedFamily()==2)return "Lower render resolution for more performance. Higher values preserve more detail.\n\n• 100% selects Native AA.\n• Named presets use the FSR runtime’s recommended resolution.";return "Lower render resolution for more performance. Higher values preserve more detail.\n\n• 100% selects DLAA.\n• Other values match a preset or select Custom.";}
 public void SetRecommendationLabel(UWidget? widget){
  if(widget==null)return;
  var label=widget as UTextBlock;
  if(label!=null && UKismetTextLibrary.Conv_TextToString(label.GetText()).Contains("Default"))label.SetText("Recommended:");
  var panel=widget as UPanelWidget;
  if(panel!=null)for(int i=0;i<panel.GetChildrenCount();i++)SetRecommendationLabel(panel.GetChildAt(i));
 }
 public void ShowRecommendation(UGameSettingDetailView details,string text){
  if(details.Box_DetailsExtension==null)return;
  UGameSettingDetailExtension_DefaultValue? footer=null;
  for(int i=0;i<HelpFooterPanels.Count;i++)if(HelpFooterPanels[i]==details && UKismetSystemLibrary.IsValid(HelpFooters[i]))footer=HelpFooters[i];
  if(footer==null && details.VisualData!=null){
   var kinds=new List<TSubclassOf<UGameSetting>>();kinds.Add(Unreal.ClassOf<UGameSettingValue>());kinds.Add(Unreal.ClassOf<UGameSettingValueDiscrete>());kinds.Add(Unreal.ClassOf<UGameSettingValueScalar>());kinds.Add(Unreal.ClassOf<UGameSetting>());
   var extensions=details.VisualData.ExtensionsForClasses;
   foreach(var kind in kinds){
    if(!extensions.ContainsKey(kind))continue;
    foreach(var soft in extensions[kind].Extensions){
     var type=UKismetSystemLibrary.LoadClassAsset_Blocking(soft);
     if(!UKismetMathLibrary.ClassIsChildOf(type,Unreal.ClassOf<UGameSettingDetailExtension_DefaultValue>()))continue;
     footer=UWidgetBlueprintLibrary.Create(this,(TSubclassOf<UUserWidget>)type,UGameplayStatics.GetPlayerController(this,0)) as UGameSettingDetailExtension_DefaultValue;
     if(footer!=null && footer.RichText_DefaultValueDetails!=null)break;
    }
    if(footer!=null && footer.RichText_DefaultValueDetails!=null)break;
   }
   if(footer!=null && footer.RichText_DefaultValueDetails!=null){HelpFooterPanels.Add(details);HelpFooters.Add(footer);}
  }
  if(footer==null || footer.RichText_DefaultValueDetails==null){details.Box_DetailsExtension.SetVisibility(ESlateVisibility.Collapsed);return;}
  if(footer.GetParent()!=details.Box_DetailsExtension)details.Box_DetailsExtension.AddChild(footer);
  footer.SetVisibility(ESlateVisibility.SelfHitTestInvisible);
  if(footer.WidgetTree!=null)SetRecommendationLabel(footer.WidgetTree.RootWidget);
  footer.RichText_DefaultValueDetails.SetText(text);
  details.Box_DetailsExtension.SetVisibility(ESlateVisibility.SelfHitTestInvisible);
 }
 public void SyncLists(){
  if(syncing)return;syncing=true;
  foreach(var p in Panels){
   if(!UKismetSystemLibrary.IsValid(p) || !p.IsVisible() || p.ListView_Settings==null)continue;var r=p.Registry as USpicewoodGameSettingRegistry_PrimaryPlayer;if(r==null || r.GfxSettings==null)continue;
   bool graphics=false;
   foreach(var setting in p.VisibleSettings){var current=setting;int depth=0;while(current!=null && depth<4){if(current==r.GfxSettings)graphics=true;current=current.SettingParent;depth++;}}
   if(!graphics){if(Model!=null && p.ListView_Settings.GetListItems().Contains(Model)){p.ListView_Settings.RemoveItem(Model);if(HeaderModel!=null)p.ListView_Settings.RemoveItem(HeaderModel);if(PresetModel!=null)p.ListView_Settings.RemoveItem(PresetModel);if(ScaleModel!=null)p.ListView_Settings.RemoveItem(ScaleModel);if(FGHeaderModel!=null)p.ListView_Settings.RemoveItem(FGHeaderModel);foreach(var ds in DisplayModels)if(ds.ControlId==5 || ds.ControlId==7)p.ListView_Settings.RemoveItem(ds);Log.Write("MOD ROWS REMOVED FROM OTHER PAGE");}continue;}
   if(Model==null){
    Probe=UWidgetBlueprintLibrary.Create(this,Unreal.ClassOf<ReconstructionRow>(),UGameplayStatics.GetPlayerController(this,0)) as ReconstructionRow;
    if(Probe==null || Probe.Skin==null || Probe.WidgetTree==null || Probe.WidgetTree.RootWidget==null){Log.Write("PROBE FAILED");continue;}
    Model=UGameplayStatics.SpawnObject(Unreal.ClassOf<ReconstructionSetting>(),r) as ReconstructionSetting;
    if(Model==null)continue;Model.LocalPlayer=r.OwningLocalPlayer;Model.OwningRegistry=r;Model.SettingParent=r.GfxSettings;
    Model.OwnedDisplayName="Upscaler";Model.OwnedHelpText=UpscalerHelpBody();
   }
   if(HeaderModel==null){HeaderModel=UGameplayStatics.SpawnObject(Unreal.ClassOf<ReconstructionHeaderSetting>(),r) as ReconstructionHeaderSetting;if(HeaderModel!=null){HeaderModel.LocalPlayer=r.OwningLocalPlayer;HeaderModel.OwningRegistry=r;HeaderModel.SettingParent=r.GfxSettings;}}
   if(PresetModel==null){PresetModel=UGameplayStatics.SpawnObject(Unreal.ClassOf<PresetSetting>(),r) as PresetSetting;if(PresetModel!=null){PresetModel.LocalPlayer=r.OwningLocalPlayer;PresetModel.OwningRegistry=r;PresetModel.SettingParent=r.GfxSettings;PresetModel.OwnedDisplayName="DLSS Preset";PresetModel.OwnedHelpText=PresetHelpBody();}}
   if(ScaleModel==null){ScaleModel=UGameplayStatics.SpawnObject(Unreal.ClassOf<ScaleSetting>(),r) as ScaleSetting;if(ScaleModel!=null){ScaleModel.LocalPlayer=r.OwningLocalPlayer;ScaleModel.OwningRegistry=r;ScaleModel.SettingParent=r.GfxSettings;ScaleModel.OwnedDisplayName="Render Scale";ScaleModel.OwnedHelpText=ScaleHelpBody();}}
   if(FGHeaderModel==null){FGHeaderModel=UGameplayStatics.SpawnObject(Unreal.ClassOf<FGHeaderSetting>(),r) as FGHeaderSetting;if(FGHeaderModel!=null){FGHeaderModel.LocalPlayer=r.OwningLocalPlayer;FGHeaderModel.OwningRegistry=r;FGHeaderModel.SettingParent=r.GfxSettings;}}
   if(p.ListView_Settings.VisualData==null)continue;var map=p.ListView_Settings.VisualData.EntryWidgetForClass;map[Unreal.ClassOf<ReconstructionHeaderSetting>()]=Unreal.ClassOf<ReconstructionHeaderRow>();map[Unreal.ClassOf<ReconstructionSetting>()]=Unreal.ClassOf<ReconstructionRow>();map[Unreal.ClassOf<PresetSetting>()]=Unreal.ClassOf<PresetRow>();map[Unreal.ClassOf<ScaleSetting>()]=Unreal.ClassOf<ScaleRow>();map[Unreal.ClassOf<FGHeaderSetting>()]=Unreal.ClassOf<FGHeaderRow>();map[Unreal.ClassOf<DisplaySetting>()]=Unreal.ClassOf<DisplayRow>();p.ListView_Settings.VisualData.EntryWidgetForClass=map;
   InsertDisplayRows(p,r);
   if(HeaderModel!=null && !p.ListView_Settings.GetListItems().Contains(HeaderModel))p.ListView_Settings.AddItem(HeaderModel);
   if(!p.ListView_Settings.GetListItems().Contains(Model)){p.ListView_Settings.AddItem(Model);Log.Write("RECONSTRUCTION APPENDED FROM EVENT");}
   if(PresetModel!=null && !p.ListView_Settings.GetListItems().Contains(PresetModel))p.ListView_Settings.AddItem(PresetModel);if(ScaleModel!=null && !p.ListView_Settings.GetListItems().Contains(ScaleModel))p.ListView_Settings.AddItem(ScaleModel);
  }
   foreach(var p in Panels){if(!UKismetSystemLibrary.IsValid(p) || !p.IsVisible() || p.ListView_Settings==null || Model==null || !p.ListView_Settings.GetListItems().Contains(Model))continue;
    if(SharedFgAvailable() || (!ProviderOwned && FGRuntime!=null && FGRuntime.Available==1)){if(FGHeaderModel!=null && !p.ListView_Settings.GetListItems().Contains(FGHeaderModel))p.ListView_Settings.AddItem(FGHeaderModel);foreach(var model in DisplayModels)if((model.ControlId==5 || (model.ControlId==7 && SharedFgAvailable())) && !p.ListView_Settings.GetListItems().Contains(model))p.ListView_Settings.AddItem(model);}
    else {if(FGHeaderModel!=null)p.ListView_Settings.RemoveItem(FGHeaderModel);foreach(var model in DisplayModels)if(model.ControlId==5 || model.ControlId==7)p.ListView_Settings.RemoveItem(model);}
   }
  syncing=false;
 }
 public void SelectOwned(UObject? item){if(item==null)return;PendingFocus=item;foreach(var p in Panels)if(p.ListView_Settings!=null && p.ListView_Settings.GetListItems().Contains(item))p.ListView_Settings.BP_NavigateToItem(item);Timer.Start(this,"RestoreOwnedFocus",0.1f,false);}
 public void RestoreOwnedFocus(){var item=PendingFocus;PendingFocus=null;if(item==null)return;foreach(var p in Panels){if(p.ListView_Settings==null || !p.IsVisible() || !p.ListView_Settings.GetListItems().Contains(item))continue;p.ListView_Settings.BP_NavigateToItem(item);foreach(var row in p.ListView_Settings.GetDisplayedEntryWidgets()){if((item==Model && row is ReconstructionRow) || (item==PresetModel && row is PresetRow) || (item==ScaleModel && row is ScaleRow) || (item is DisplaySetting && row is DisplayRow && (row as DisplayRow).Setting==item))row.SetUserFocus(UGameplayStatics.GetPlayerController(this,0));}}}
 public void ShowReconstructionHelp(){displayHelp=-1;HelpKind=0;OwnHelpVisible=true;ShowHelp();}
 public void ShowPresetHelp(){displayHelp=-1;HelpKind=1;OwnHelpVisible=true;ShowHelp();}
 public void ShowScaleHelp(){displayHelp=-1;HelpKind=2;OwnHelpVisible=true;ShowHelp();}
 public string ProviderStatus(){
  if(!SharedControls() || ProviderMenu==null)return "Settings unavailable.";
  string saved=ProviderMenu.Status();if(saved!="")return saved;
  if(SelectedProvider()==0)return "";
  if(ProviderRuntime!=null && ProviderRuntime.State!=null && SelectedProvider()==2){var state=ProviderRuntime.State;
   if(state.W11!=0)return "Upscaler unavailable; Native is active.";
   if(state.W10==5)return "";
  }
  if(SelectedProvider()==1 && Runtime!=null && Saved!=null && Runtime.RequestedRevision==Saved.Revision){if(Runtime.Phase==2)return "";if(Runtime.Phase==3)return "Upscaler unavailable; Native is active.";}
  return "Resumes during gameplay.";
 }
 public void ShowHelp(){
  if(displayHelp>=0){ShowDisplayHelp(displayHelp);return;}
  if(Model==null || !OwnHelpVisible)return;string title="Upscaler";string help=UpscalerHelpBody();if(HelpKind==1 && PresetModel!=null){title=PresetTitle();help=PresetHelpBody();}if(HelpKind==2 && ScaleModel!=null){title="Render Scale";help=ScaleHelpBody();}
  bool fsr=SharedControls() && SelectedFamily()==2;
  if(HelpKind==2)help+="\nCurrent minimum: "+MinimumScalePercent+"%.";if(outputWidth>0 && !ScaleSupported(10000))help+=fsr?"\nFSR is unavailable at this output resolution. Supported output: 640 x 360 to 7680 x 4320.":"\nDLSS is unavailable at this output resolution. Supported output: 640 x 360 to 3840 x 2160.";
  foreach(var p in Panels){if(p.Details_Settings==null || p.ListView_Settings==null || !p.ListView_Settings.GetListItems().Contains(Model))continue;
   var d=p.Details_Settings;if(d.Text_SettingName!=null)d.Text_SettingName.SetText(title);
   
   if(d.RichText_DynamicDetails!=null){string status="Resume gameplay to apply.";if(Runtime!=null && Saved!=null && Runtime.RequestedRevision==Saved.Revision){if(Runtime.Phase==1 || Runtime.Phase==4)status="Applying upscaler change...";if(Runtime.Phase==2)status=Saved.RenderContextReady==0 && Saved.ReconstructionMode!=0?"DLSS activates in gameplay.":"";if(Runtime.Phase==3)status=Runtime.ErrorCode==2?"This render scale is unsupported; native anti-aliasing is active.":"DLSS unavailable; native anti-aliasing is active.";}d.RichText_DynamicDetails.SetText("");if(d.RichText_Description!=null)d.RichText_Description.SetText(help+((ProviderOwned?ProviderStatus():status)==""?"":"\n\n"+(ProviderOwned?ProviderStatus():status)));}if(d.RichText_WarningDetails!=null)d.RichText_WarningDetails.SetText("");if(d.RichText_DisabledDetails!=null)d.RichText_DisabledDetails.SetText("");
   string recommendation=HelpKind==0?(SharedControls() && SelectedFamily()==2?"AMD FSR":"NVIDIA DLSS"):(HelpKind==1?"Quality":"67%");
   if(fsr && HelpKind==2)recommendation="Quality preset";
   if(HelpKind!=0 && MinimumScalePercent>67)recommendation=FullResolutionName();
   if(outputWidth>0 && !ScaleSupported(10000))recommendation="Native at this output resolution";
   if(!fsr && Runtime!=null && Runtime.RequestedRevision==(Saved==null?0:Saved.Revision) && Runtime.Phase==3)recommendation="Native while DLSS is unavailable";
   ShowRecommendation(d,recommendation);
  }
 }
 // Preset IDs 0-4 retain their saved meanings; DLAA is appended as 5.
 public int PresetSliderScale(int preset){if(preset==5)return 10000;if(preset==0)return 6700;if(preset==1)return 5800;if(preset==2)return 5000;if(preset==3)return 3300;return Saved==null?6700:Saved.CustomScaleBasisPoints;}
 public int PresetAtScale(int scale){if(scale==10000)return 5;if(scale==6700)return 0;if(scale==5800)return 1;if(scale==5000)return 2;if(scale==3300)return 3;return 4;}
 // Use output viewport dimensions, never the reduced source dimensions.
 public bool ScaleSupported(int scale){
  if(outputWidth<=0 || outputHeight<=0)return scale>=1700;
  if(SharedControls() && SelectedFamily()==2){if(outputWidth>7680 || outputHeight>4320)return false;}else if(outputWidth>3840 || outputHeight>2160)return false;
  int divisor=10000;int numerator=scale;
  if(!SharedControls() || SelectedFamily()==1){if(scale==6700){divisor=3;numerator=2;}if(scale==3300){divisor=3;numerator=1;}}
  return (long)outputWidth*numerator>=640L*divisor && (long)outputHeight*numerator>=360L*divisor;
 }
 public int ClampScale(int scale){if(scale<MinimumScalePercent*100)scale=MinimumScalePercent*100;if(scale>10000)scale=10000;return scale;}
 public void RefreshResolutionBounds(){
  var player=UGameplayStatics.GetPlayerController(this,0);if(player==null)return;
  player.GetViewportSize(out int width,out int height);if(width<=0 || height<=0 || (width==outputWidth && height==outputHeight))return;
  outputWidth=width;outputHeight=height;MinimumScalePercent=100;
  for(int percent=1;percent<=100;percent++){if(ScaleSupported(percent*100)){MinimumScalePercent=percent;break;}}
  if(!ProviderOwned && Saved!=null && Saved.SchemaVersion==4){
   bool supported=ScaleSupported(10000);
   if(!supported && Saved.ReconstructionMode!=0){Saved.ReconstructionMode=0;UpdateScale();Saved.Revision++;Persist();}
   else if(supported && !ScaleSupported(Saved.CustomScaleBasisPoints)){
    int scale=ClampScale(Saved.CustomScaleBasisPoints);Saved.CustomScaleBasisPoints=scale;Saved.SRPreset=PresetAtScale(scale);
    if(Saved.SRPreset==4)Saved.LastCustomScaleBasisPoints=scale;
    if(Saved.ReconstructionMode!=0)Saved.ReconstructionMode=Saved.SRPreset==5?1:2;
    UpdateScale();Saved.Revision++;Persist();
   }
  }
  RefreshRows();ShowHelp();
 }
 public void ChangeMode(int direction){
  if(ProviderOwned){if(SharedControls() && ProviderMenu!=null && direction!=0){int next=SelectedProvider()+direction;if(next<0)next=2;if(next>2)next=0;ProviderMenu.SelectMenuSr(next);SyncLists();RefreshRows();}return;}
  if(Saved==null || Saved.SchemaVersion!=4 || direction==0)return;
  if(!ScaleSupported(10000))return;
  Saved.ReconstructionMode=Saved.ReconstructionMode==0?(Saved.SRPreset==5?1:2):0;UpdateScale();Saved.Revision++;Persist();RefreshRows();
 }
 public void UpdateScale(){if(Saved==null)return;int scale=10000;if(Saved.ReconstructionMode==2){scale=6667;if(Saved.SRPreset==1)scale=5800;if(Saved.SRPreset==2)scale=5000;if(Saved.SRPreset==3)scale=3333;if(Saved.SRPreset==4)scale=Saved.CustomScaleBasisPoints;}Saved.RenderScaleBasisPoints=scale;}
 public void ChangePreset(int direction){if(ProviderOwned){if(SharedControls() && ProviderMenu!=null){int family=SelectedFamily(),quality=SelectedQuality()+direction;if(quality<0)quality=5;if(quality>5)quality=0;ProviderMenu.SelectQuality(family,quality);ProviderMenu.SelectMenuSr(family);RefreshRows();}return;}if(Saved==null || Saved.SchemaVersion!=4)return;int index=(Saved.SRPreset==5?0:Saved.SRPreset+1)+direction;if(index<0)index=5;if(index>5)index=0;int p=index==0?5:index-1;for(int attempt=0;attempt<6;attempt++){int candidate=p==4?ClampScale(Saved.LastCustomScaleBasisPoints):PresetSliderScale(p);if(ScaleSupported(candidate))break;index+=direction;if(index<0)index=5;if(index>5)index=0;p=index==0?5:index-1;}if(!ScaleSupported(10000))return;Saved.CustomScaleBasisPoints=p==4?ClampScale(Saved.LastCustomScaleBasisPoints):PresetSliderScale(p);p=PresetAtScale(Saved.CustomScaleBasisPoints);Saved.SRPreset=p;Saved.ReconstructionMode=p==5?1:2;UpdateScale();Saved.Revision++;Persist();RefreshRows();}
 public void ChangeScale(int scale){if(ProviderOwned){if(SharedControls() && ProviderMenu!=null){int family=SelectedFamily();ProviderMenu.SelectCustomScale(family,ClampScale(scale));ProviderMenu.SelectMenuSr(family);RefreshRows();}return;}if(Saved==null || Saved.SchemaVersion!=4)return;if(!ScaleSupported(10000))return;scale=ClampScale(scale);int p=PresetAtScale(scale);int mode=p==5?1:2;if(Saved.CustomScaleBasisPoints==scale && Saved.ReconstructionMode==mode && Saved.SRPreset==p)return;Saved.CustomScaleBasisPoints=scale;if(p==4)Saved.LastCustomScaleBasisPoints=scale;Saved.SRPreset=p;Saved.ReconstructionMode=mode;UpdateScale();Saved.Revision++;Persist();RefreshRows();}
 public void ResetOwned(){if(ProviderOwned){if(SharedControls() && ProviderMenu!=null){ProviderMenu.ResetDefaults();RefreshRows();}return;}if(FGSaved!=null){FGSaved.Mode=0;FGSaved.Revision++;PersistFG();}if(Saved==null || Saved.SchemaVersion!=4)return;Saved.ReconstructionMode=0;Saved.SRPreset=0;Saved.CustomScaleBasisPoints=6700;Saved.LastCustomScaleBasisPoints=7700;Saved.RenderScaleBasisPoints=10000;Saved.NativeFallback=true;Saved.Revision++;Persist();if(DisplaySaved!=null){DisplaySaved.HDROutput=0;DisplaySaved.PeakNits=1000;DisplaySaved.PaperWhiteNits=203;DisplaySaved.UINits=203;DisplaySaved.ReflexMode=1;DisplaySaved.Revision++;PersistDisplay();}RefreshRows();Log.Write("OWNED RESET");}
 public void Persist(){if(Saved!=null)Log.Write("SAVE="+UGameplayStatics.SaveGameToSlot(Saved,"MCD2GraphicsSettings",0)+" revision="+Saved.Revision+" mode="+Saved.ReconstructionMode);}
 public void RefreshRows(){RefreshDisplayRows();UWidgetBlueprintLibrary.GetAllWidgetsOfClass(this,out List<UUserWidget> rows,Unreal.ClassOf<ReconstructionRow>(),false);foreach(var w in rows){var row=w as ReconstructionRow;if(row!=null)row.Refresh();}UWidgetBlueprintLibrary.GetAllWidgetsOfClass(this,out List<UUserWidget> presets,Unreal.ClassOf<PresetRow>(),false);foreach(var w in presets){var row=w as PresetRow;if(row!=null)row.Refresh();}UWidgetBlueprintLibrary.GetAllWidgetsOfClass(this,out List<UUserWidget> scales,Unreal.ClassOf<ScaleRow>(),false);foreach(var w in scales){var row=w as ScaleRow;if(row!=null)row.Refresh();}}

 public void InitializeDisplay(){
  var settings=UGameUserSettings.GetGameUserSettings();displayHDRSupported=settings!=null && settings.SupportsHDRDisplayOutput();
  DisplaySaved=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsDisplaySettings",0) as DisplaySettingsSave;
  if(DisplaySaved==null){DisplaySaved=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<DisplaySettingsSave>()) as DisplaySettingsSave;
   if(DisplaySaved!=null){DisplaySaved.SchemaVersion=1;DisplaySaved.Revision=1;DisplaySaved.HDROutput=settings!=null && settings.IsHDREnabled()?1:0;int n=settings==null?1000:settings.GetCurrentHDRDisplayNits();if(n<100 || n>10000)n=1000;DisplaySaved.PeakNits=n/10*10;DisplaySaved.PaperWhiteNits=203;DisplaySaved.UINits=203;DisplaySaved.ReflexMode=1;PersistDisplay();}}
  // Never expose capability or acknowledge calibration from an earlier session.
  {DisplayRuntime=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<DisplayRuntimeSave>()) as DisplayRuntimeSave;if(DisplayRuntime!=null){DisplayRuntime.SchemaVersion=1;UGameplayStatics.SaveGameToSlot(DisplayRuntime,"MCD2GraphicsDisplayRuntime",0);}}
 }
 public void PersistDisplay(){if(DisplaySaved!=null)UGameplayStatics.SaveGameToSlot(DisplaySaved,"MCD2GraphicsDisplaySettings",0);}
 public int SelectedFgProvider(){return SharedControls() && ProviderMenu!=null?ProviderMenu.Value(20):(FGSaved==null?0:FGSaved.FGProvider);}
 public bool SharedFgAvailable(){return SharedControls() && FGRuntime!=null && FGRuntime.AvailableProviders>0 && FGRuntime.AvailableProviders<=3;}
 public bool SelectedFgAvailable(){if(!SharedControls())return FGRuntime!=null && FGRuntime.Available==1;if(!SharedFgAvailable() || FGRuntime==null)return false;int fg=SelectedFgProvider(),sr=SelectedProvider();return (fg==0 && sr==1 && (FGRuntime.AvailableProviders&1)!=0) || (fg==1 && (sr==1 || sr==2) && (FGRuntime.AvailableProviders&2)!=0);}
 public int DisplayValue(int id){if(id==7)return SelectedFgProvider();if(id==6)return SharedControls() && ProviderMenu!=null && (ProviderMenu.Value(23)==0 || ProviderMenu.Value(23)==2) && ProviderMenu.Value(24)==1?1:0;if(SharedControls() && ProviderMenu!=null){if(id==5)return ProviderMenu.Value(19);if(id==0)return ProviderMenu.Value(25);if(id==1)return ProviderMenu.Value(26);if(id==2)return ProviderMenu.Value(27);if(id==3)return ProviderMenu.Value(28);return ProviderMenu.Value(24);}if(DisplaySaved==null)return 0;if(id==5)return FGSaved==null?0:FGSaved.Mode;if(id==0)return DisplaySaved.HDROutput;if(id==1)return DisplaySaved.PeakNits;if(id==2)return DisplaySaved.PaperWhiteNits;if(id==3)return DisplaySaved.UINits;return DisplaySaved.ReflexMode;}
 public string DisplayLabel(int id){if(id==7)return "FG Provider";if(id==6)return "AMD Anti-Lag 2";if(id==5)return "Frame Generation";if(id==0)return "HDR Output";if(id==1)return "HDR Peak Brightness";if(id==2)return "HDR Paper White";if(id==3)return "HDR UI Brightness";return "NVIDIA Reflex";}
 public void ChangeDisplay(int id,int direction){if(id==6 && !ProviderOwned)return;if(ProviderOwned){
  if(!SharedControls() || ProviderMenu==null || direction==0)return;
  if(id==7){if(!SharedFgAvailable())return;int provider=SelectedFgProvider()==0?1:0;if(FGRuntime==null || (FGRuntime.AvailableProviders&(provider==0?1:2))==0)return;ProviderMenu.SelectFg(provider,0,2,false);}
  else if(id==5){if(!SelectedFgAvailable())return;ProviderMenu.SelectFg(SelectedFgProvider(),0,2,ProviderMenu.Value(19)==0);}
  else if(id==6){if(!AmdLatencyAvailable())return;ProviderMenu.SelectAntiLag2(DisplayValue(6)==0);}
  else if(id==4){if(DisplayRuntime==null || DisplayRuntime.ReflexAvailable!=1 || DisplayRuntime.ReflexFault!=0)return;int mode=ProviderMenu.Value(24)+direction;if(mode<0)mode=2;if(mode>2)mode=0;ProviderMenu.SelectLatency(1,mode);}
  else {int enabled=ProviderMenu.Value(25),peak=ProviderMenu.Value(26),paper=ProviderMenu.Value(27),ui=ProviderMenu.Value(28);
   if(id==0){if(!displayHDRSupported && enabled==0)return;enabled=enabled==0?1:0;}
   else {if(enabled==0)return;int value=DisplayValue(id)+direction*(id==1?10:1);int min=id==1?100:48,max=id==1?10000:500;if(value<min)value=min;if(value>max)value=max;if(id==1)peak=value;if(id==2)paper=value;if(id==3)ui=value;}
   ProviderMenu.SelectHdr(enabled==1,peak,paper,ui);
  }RefreshDisplayRows();return;
 }if(DisplaySaved==null || DisplaySaved.SchemaVersion!=1 || direction==0)return;
  if(id==5){if(FGSaved==null || FGRuntime==null || FGRuntime.Available!=1)return;FGSaved.Mode=FGSaved.Mode==0?1:0;FGSaved.Revision++;PersistFG();RefreshDisplayRows();return;}
  if(id==0){if(!displayHDRSupported && DisplaySaved.HDROutput==0)return;DisplaySaved.HDROutput=DisplaySaved.HDROutput==0?1:0;}
  else if(id==4){if(DisplayRuntime==null || DisplayRuntime.ReflexAvailable!=1 || DisplayRuntime.ReflexFault!=0)return;int n=DisplaySaved.ReflexMode+direction;if(n<0)n=2;if(n>2)n=0;DisplaySaved.ReflexMode=n;}
  else {if(DisplaySaved.HDROutput==0)return;int n=DisplayValue(id)+direction*(id==1?10:1);int min=id==1?100:48;int max=id==1?10000:500;if(n<min)n=min;if(n>max)n=max;if(id==1)DisplaySaved.PeakNits=n;if(id==2)DisplaySaved.PaperWhiteNits=n;if(id==3)DisplaySaved.UINits=n;}
  DisplaySaved.Revision++;PersistDisplay();RefreshDisplayRows();
 }
 public void PollDisplay(){
  if(DisplaySaved==null || DisplaySaved.SchemaVersion!=1)return;
  var state=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsDisplayRuntime",0) as DisplayRuntimeSave;
  bool supportChanged=state!=null && (DisplayRuntime==null || state.ReflexAvailable!=DisplayRuntime.ReflexAvailable || state.SessionId!=DisplayRuntime.SessionId || state.ReflexFault!=DisplayRuntime.ReflexFault);
  if(state!=null && state.SchemaVersion==1)DisplayRuntime=state;
  bool amdVisible=AmdLatencyAvailable();if(supportChanged || amdVisible!=amdMenuVisible){amdMenuVisible=amdVisible;SyncLists();}
  if(displayAppliedRevision!=DisplaySaved.Revision && DisplayRuntime!=null && DisplayRuntime.HDRRevision==DisplaySaved.Revision && DisplayRuntime.HDRRestartRequired==0){var settings=UGameUserSettings.GetGameUserSettings();if(settings!=null){displayHDRSupported=settings.SupportsHDRDisplayOutput();if(displayHDRSupported || DisplaySaved.HDROutput==0){
    if(DisplaySaved.HDROutput==1){UKismetSystemLibrary.ExecuteConsoleCommand(this,"r.AllowHDR 1",UGameplayStatics.GetPlayerController(this,0));UKismetSystemLibrary.ExecuteConsoleCommand(this,"r.HDR.Display.OutputDevice 3",UGameplayStatics.GetPlayerController(this,0));UKismetSystemLibrary.ExecuteConsoleCommand(this,"r.HDR.Display.ColorGamut 2",UGameplayStatics.GetPlayerController(this,0));UKismetSystemLibrary.ExecuteConsoleCommand(this,"r.HDR.UI.CompositeMode 1",UGameplayStatics.GetPlayerController(this,0));}
    // Reflex/paper-white revisions do not change the native output mode.
    // Avoid recreating display resources for an already matching HDR state.
    if(settings.IsHDREnabled()!=(DisplaySaved.HDROutput==1) || settings.GetCurrentHDRDisplayNits()!=DisplaySaved.PeakNits){Log.Write("DISPLAY APPLY BEGIN revision="+DisplaySaved.Revision);settings.EnableHDRDisplayOutput(DisplaySaved.HDROutput==1,DisplaySaved.PeakNits);settings.SaveSettings();Log.Write("DISPLAY APPLY END");}displayAppliedRevision=DisplaySaved.Revision;
  }}}
  RefreshDisplayRows();if(displayHelp>=0){if(GraphicsPageOpen())ShowDisplayHelp(displayHelp);else {displayHelp=-1;OwnHelpVisible=false;}}
 }
 public void InsertDisplayRows(USpicewoodGameSettingPanel panel,USpicewoodGameSettingRegistry_PrimaryPlayer registry){
  if(panel.ListView_Settings==null || registry.GfxSettings==null)return;
  if(DisplayModels.Count==0){for(int id=0;id<8;id++){var model=UGameplayStatics.SpawnObject(Unreal.ClassOf<DisplaySetting>(),registry) as DisplaySetting;if(model!=null){model.ControlId=id;model.LocalPlayer=registry.OwningLocalPlayer;model.OwningRegistry=registry;model.SettingParent=registry.GfxSettings;DisplayModels.Add(model);}}}
  var list=panel.ListView_Settings.GetListItems();var result=new List<UObject>();bool hdrInserted=false;bool reflexInserted=false;
  foreach(var item in list){if(item is DisplaySetting){var own=item as DisplaySetting;if(own!=null && (own.ControlId==5 || own.ControlId==7) && (SharedFgAvailable() || (!ProviderOwned && own.ControlId==5 && FGRuntime!=null && FGRuntime.Available==1)))result.Add(item);continue;}result.Add(item);if(item==Model || item==HeaderModel || item==PresetModel || item==ScaleModel)continue;var setting=item as UGameSetting;if(setting==null)continue;string label=UKismetTextLibrary.Conv_TextToString(setting.GetDisplayName());string name=UKismetSystemLibrary.GetObjectName(setting);
   if(!hdrInserted && (label=="Brightness" || name.Contains("Brightness"))){foreach(var model in DisplayModels)if(model.ControlId<4)result.Add(model);hdrInserted=true;}
   if(!reflexInserted && (label=="FPS Limit" || label.Contains("Frame Rate Limit") || name.Contains("FrameRateLimit") || name.Contains("FPSLimit"))){foreach(var model in DisplayModels)if(model.ControlId==4 && ((DisplayRuntime!=null && DisplayRuntime.ReflexAvailable==1 && DisplayRuntime.ReflexFault==0) || list.Contains(model)))result.Add(model);foreach(var model in DisplayModels)if(model.ControlId==6 && AmdLatencyAvailable())result.Add(model);reflexInserted=true;}
  }
  // Fail closed when the native anchor cannot be identified; no misplaced giant mod block.
  bool changed=list.Count!=result.Count;if(!changed){for(int i=0;i<list.Count;i++)if(list[i]!=result[i])changed=true;}
  if(changed)panel.ListView_Settings.BP_SetListItems(result);RefreshDisplayRows();
 }
 public void RefreshDisplayRows(){UWidgetBlueprintLibrary.GetAllWidgetsOfClass(this,out List<UUserWidget> rows,Unreal.ClassOf<DisplayRow>(),false);foreach(var w in rows){var row=w as DisplayRow;if(row!=null && UKismetSystemLibrary.IsValid(row) && row.IsVisible())row.Refresh();}}
 public void InitializeFG(){
  FGSaved=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsFGSettings",0) as FGSettingsSave;
  if(FGSaved==null)FGSaved=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<FGSettingsSave>()) as FGSettingsSave;
  if(FGSaved!=null){if(FGSaved.SchemaVersion!=1 || FGSaved.Mode<0 || FGSaved.Mode>1 || FGSaved.FGProvider<0 || FGSaved.FGProvider>1)FGSaved.Mode=0;FGSaved.SchemaVersion=1;FGSaved.ContextReady=0;FGSaved.SessionId=0;FGSaved.Revision++;PersistFG();}
  FGRuntime=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<FGRuntimeSave>()) as FGRuntimeSave;
  if(FGRuntime!=null){FGRuntime.SchemaVersion=1;UGameplayStatics.SaveGameToSlot(FGRuntime,"MCD2GraphicsFGRuntime",0);}
 }
 public void PersistFG(){if(FGSaved!=null)UGameplayStatics.SaveGameToSlot(FGSaved,"MCD2GraphicsFGSettings",0);}
 public void PollFG(){
  if(FGSaved==null)return;
  var state=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsFGRuntime",0) as FGRuntimeSave;
  bool changed=state!=null && (FGRuntime==null || state.Available!=FGRuntime.Available || state.AvailableProviders!=FGRuntime.AvailableProviders || state.SessionId!=FGRuntime.SessionId);
  if(state!=null && state.SchemaVersion==1)FGRuntime=state;
  int ready=Saved!=null && Saved.RenderContextReady==1 && !UGameplayStatics.IsGamePaused(this)?1:0;
  foreach(var layer in Layers){if(!UKismetSystemLibrary.IsValid(layer))continue;if(UKismetSystemLibrary.GetObjectName(layer)=="Activity_Stack")continue;var active=layer.DisplayedWidget;if(active==null || !UKismetSystemLibrary.IsValid(active) || !active.IsActivated())continue;string name=UKismetSystemLibrary.GetObjectName(active);if(!name.StartsWith("W_PlayerHUD_Activatable_C_") && !name.StartsWith("W_HotbarContainer_Activatable_C_") && !name.StartsWith("W_InGameNavigation_Activatable_C_")){ready=0;}}
  // The navigation root stays active around its Inventory/System children.
  // Check those children's activation too; a root-only gate admits menus.
  foreach(var active in LifecycleWidgets){if(!UKismetSystemLibrary.IsValid(active) || !active.IsActivated())continue;string name=UKismetSystemLibrary.GetObjectName(active);if(name=="Hotbar" || name=="MiniInventory" || name.StartsWith("W_ActivityLayer_Activatable_C_") || name.StartsWith("W_PlayerHUD_Activatable_C_") || name.StartsWith("W_HotbarContainer_Activatable_C_") || name.StartsWith("W_InGameNavigation_Activatable_C_") || name.Contains("Notification_Activatable_C_"))continue;ready=0;}
  int session=FGRuntime==null?0:FGRuntime.SessionId;
  bool intentChanged=FGSaved.ContextReady!=ready || FGSaved.SessionId!=session;
  if(intentChanged){FGSaved.ContextReady=ready;FGSaved.SessionId=session;FGSaved.Revision++;Log.Write("FG CONTEXT ready="+ready);}
  // A current-process heartbeat prevents stale gameplay intent after travel or UI failure.
  if(FGSaved.Mode==1 || intentChanged)PersistFG();UpdateFoliageVelocity(ready);if(changed)SyncLists();if((displayHelp==5 || displayHelp==7) && GraphicsPageOpen())ShowDisplayHelp(displayHelp);
 }
 public void RestoreFoliageVelocity(){
  if(!foliageVelocityOwned || foliageRestoreAttempts>=3)return;
  // Restore only our value; a later console/user/mod change takes precedence.
  if(UKismetSystemLibrary.GetConsoleVariableStringValue("r.Velocity.EnableVertexDeformation")!="" && UKismetSystemLibrary.GetConsoleVariableIntValue("r.Velocity.EnableVertexDeformation")==1){
   foliageRestoreAttempts++;UKismetSystemLibrary.ExecuteConsoleCommand(this,"r.Velocity.EnableVertexDeformation "+foliageVelocityOriginal,UGameplayStatics.GetPlayerController(this,0));
   if(UKismetSystemLibrary.GetConsoleVariableIntValue("r.Velocity.EnableVertexDeformation")!=foliageVelocityOriginal){Log.Write("SR FOLIAGE RESTORE FAILED");return;}
   Log.Write("SR FOLIAGE RESTORED");
  }
  foliageVelocityOwned=false;
 }
 public void UpdateFoliageVelocity(int ready){
  // Vertex deformation velocities feed both temporal SR providers, independently of FG/HDR.
  // Only a current renderer acknowledgement may acquire or retain the override.
  bool eligible=Saved!=null && Saved.SchemaVersion==4 && Saved.RenderContextReady==1 && Saved.ReconstructionMode>0 && Runtime!=null && Runtime.SchemaVersion==1 && Runtime.SessionId>0 && Saved.RenderContextSessionId==Runtime.SessionId && Runtime.RequestedRevision==Saved.Revision && Runtime.ErrorCode==0 && Runtime.Phase!=3;
  bool active=Runtime!=null && Runtime.Phase==2;
  if(ProviderOwned){
   if(!SharedControls()){eligible=false;active=false;}
   else if(SelectedProvider()==2){
    eligible=ProviderRuntime!=null && ProviderMenu!=null && ProviderRuntime.FoliageMotionReady(ProviderMenu,false);
    active=ProviderRuntime!=null && ProviderMenu!=null && ProviderRuntime.FoliageMotionReady(ProviderMenu,true);
   }else if(SelectedProvider()!=1){eligible=false;active=false;}
  }
  if(!eligible){RestoreFoliageVelocity();foliageVelocityBlocked=false;return;}
  if(foliageVelocityOwned){
   if(UKismetSystemLibrary.GetConsoleVariableStringValue("r.Velocity.EnableVertexDeformation")=="" || UKismetSystemLibrary.GetConsoleVariableIntValue("r.Velocity.EnableVertexDeformation")!=1){foliageVelocityOwned=false;foliageVelocityBlocked=true;}
   return;
  }
  // Acquire in verified gameplay only. Keep the lease through temporary menus.
  if(foliageVelocityBlocked || ready!=1 || !active)return;
  if(UKismetSystemLibrary.GetConsoleVariableStringValue("r.Velocity.EnableVertexDeformation")=="" || UKismetSystemLibrary.GetConsoleVariableIntValue("r.VelocityOutputPass")!=2){foliageVelocityBlocked=true;return;}
  int original=UKismetSystemLibrary.GetConsoleVariableIntValue("r.Velocity.EnableVertexDeformation");
  if(original==1)return;
  if(original!=0 && original!=2){foliageVelocityBlocked=true;return;}
  UKismetSystemLibrary.ExecuteConsoleCommand(this,"r.Velocity.EnableVertexDeformation 1",UGameplayStatics.GetPlayerController(this,0));
  if(UKismetSystemLibrary.GetConsoleVariableIntValue("r.Velocity.EnableVertexDeformation")==1){foliageVelocityOriginal=original;foliageRestoreAttempts=0;foliageVelocityOwned=true;Log.Write("SR FOLIAGE VELOCITY ENABLED");}else{foliageVelocityBlocked=true;Log.Write("SR FOLIAGE APPLY FAILED");}
 }
 public void ShowDisplayHelp(int id){displayHelp=id;OwnHelpVisible=true;string help="";
  if(id==7)help="Choose the frame-generation provider independently of the upscaler.\n\n• NVIDIA DLSS\n• AMD FSR\n\nRestart after changing providers.";
  if(id==5)help="Generate an extra frame between rendered frames for smoother motion.\n\n• Off — rendered frames only\n• On — rendered and generated frames\n\nRestart after changing modes.";
  if(id==0)help="Enable HDR for a compatible display with system HDR turned on.\n\n• Off — standard dynamic range\n• On — HDR output";
  if(id==1)help="Match the peak brightness of your display’s HDR mode. Use its calibrated value.";
  if(id==2)help="Adjust the brightness of ordinary scene white in HDR. Raise it for a brighter room.";
  if(id==3)help="Adjust menu and text brightness in HDR without changing the scene.";
  if(id==6)help="Reduce input latency on supported AMD GPUs.\n\n• Off — disabled\n• On — lower latency";
  if(id==4)help="Reduce input latency on supported NVIDIA GPUs.\n\n• Off — disabled\n• On — lower latency\n• On + Boost — lower latency and higher GPU clocks; uses more power";
  if(id==6 && !AmdLatencyAvailable())help+="\n\nUnavailable in this session.";
  if(id==4 && (DisplayRuntime==null || DisplayRuntime.ReflexAvailable!=1 || DisplayRuntime.ReflexFault!=0))help+="\n\nUnavailable in this session.";
  if(id<4){if(!displayHDRSupported)help+="\n\nEnable system HDR to use this setting.";if(DisplayRuntime!=null && DisplayRuntime.HDRRestartRequired==1)help+="\n\nRestart to apply.";}
  string recommendation=id==7?(SelectedFgProvider()==1?"AMD FSR":"NVIDIA DLSS"):id==5?"Off":id==0?(displayHDRSupported?"On":"Off"):(id==1?"Display peak":(id==2 || id==3?"203 nits":"On"));
  if(id==6 && !AmdLatencyAvailable())recommendation="Unavailable";
  if(id==4 && (DisplayRuntime==null || DisplayRuntime.ReflexAvailable!=1 || DisplayRuntime.ReflexFault!=0))recommendation="Unavailable";
  foreach(var p in Panels){if(!UKismetSystemLibrary.IsValid(p) || !p.IsVisible() || p.Details_Settings==null || p.ListView_Settings==null || Model==null || !p.ListView_Settings.GetListItems().Contains(Model))continue;var d=p.Details_Settings;if(d.Text_SettingName!=null)d.Text_SettingName.SetText(DisplayLabel(id));if(d.RichText_Description!=null)d.RichText_Description.SetText(help);if(d.RichText_DynamicDetails!=null)d.RichText_DynamicDetails.SetText(id==6?AmdLatencyStatus():(id==5 || id==7)?FGStatus():id==4 && DisplayRuntime!=null?"Active mode: "+(DisplayRuntime.ReflexMode==0?"Off":DisplayRuntime.ReflexMode==1?"On":"On + Boost"):"");if(d.RichText_WarningDetails!=null)d.RichText_WarningDetails.SetText("");if(d.RichText_DisabledDetails!=null)d.RichText_DisabledDetails.SetText("");ShowRecommendation(d,recommendation);}
 }

 public string AmdLatencyStatus(){
  if(!AmdLatencyAvailable() || ProviderMenu==null || ProviderMenu.Authority==null || DisplayRuntime==null)return "Unavailable in this session.";
  int applied=ProviderRuntimeClient.AmdLatencyApplied(ProviderMenu.Authority.W19,DisplayRuntime.AmdAntiLagRevision,DisplayRuntime.AmdAntiLagMode);
  return applied<0?"Applying change...":"Active mode: "+(applied==0?"Off":"On");
 }
 public string FGStatus(){
  if(FGRuntime==null || !SelectedFgAvailable())return "Unavailable with the current upscaler.";
  if(FGRuntime.RestartRequired==1)return "Restart to apply.";
  if(DisplayValue(5)==0)return "Off.";
  if(FGRuntime.Phase==6)return "Frame generation stopped after a runtime error.";
  if(DisplaySaved==null || DisplaySaved.HDROutput==0)return "Enable HDR to use this mode.";
  return FGRuntime.Active==1?"Active.":"Resumes during gameplay.";
 }

 protected override void ReceiveEndPlay(EEndPlayReason reason){
  if(ProviderRuntime!=null)ProviderRuntime.Stop();Timer.Stop(this,"PollProviders");
  RestoreFoliageVelocity();
  foreach(var footer in HelpFooters)if(UKismetSystemLibrary.IsValid(footer))footer.RemoveFromParent();HelpFooters.Clear();HelpFooterPanels.Clear();
  if(Saved!=null && Saved.SchemaVersion==4 && Saved.RenderContextReady!=0){Saved.RenderContextReady=0;Saved.RenderContextSessionId=0;if(!ProviderOwned)Saved.Revision++;Persist();}
  if(FGSaved!=null){FGSaved.ContextReady=0;FGSaved.SessionId=0;FGSaved.Revision++;PersistFG();}Timer.Stop(this,"PollFG");
  Timer.Stop(this,"Discover");Timer.Stop(this,"SyncLists");Timer.Stop(this,"PollRenderer");Timer.Stop(this,"PollDisplay");Timer.Stop(this,"RestoreOwnedFocus");foreach(var button in ResetButtons)button.OnButtonBaseClicked-=ResetConfirmed;foreach(var active in LifecycleWidgets){active.BP_OnWidgetActivated-=LifecycleChanged;active.BP_OnWidgetDeactivated-=LifecycleChanged;}foreach(var p in Panels){if(p.ListView_Settings==null)continue;p.ListView_Settings.BP_OnEntriesGenerated-=EntriesGenerated;p.ListView_Settings.BP_OnEntryReleased-=EntryReleased;p.ListView_Settings.BP_OnItemSelectionChanged-=SelectionChanged;p.ListView_Settings.BP_OnItemIsHoveredChanged-=HoverChanged;if(Model!=null)p.ListView_Settings.RemoveItem(Model);if(HeaderModel!=null)p.ListView_Settings.RemoveItem(HeaderModel);if(PresetModel!=null)p.ListView_Settings.RemoveItem(PresetModel);if(ScaleModel!=null)p.ListView_Settings.RemoveItem(ScaleModel);if(FGHeaderModel!=null)p.ListView_Settings.RemoveItem(FGHeaderModel);foreach(var ds in DisplayModels)p.ListView_Settings.RemoveItem(ds);}Log.Write("LIST EVENTS UNBOUND");
 }
}
