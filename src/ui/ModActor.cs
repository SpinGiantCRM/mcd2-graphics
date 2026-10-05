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
  if(Skin!=null && Manager!=null && Manager.Saved!=null && Skin.Rotator_SettingValue!=null)Skin.Rotator_SettingValue.SetSelectedItem(Manager.Saved.ReconstructionMode==0?0:1);
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
  if(Skin!=null && Manager!=null && Manager.Saved!=null && Skin.Rotator_SettingValue!=null)Skin.Rotator_SettingValue.SetSelectedItem(Manager.Saved.SRPreset==5?0:Manager.Saved.SRPreset+1);
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
 public void Refresh(){if(Skin==null || Manager==null || Manager.Saved==null)return;int span=100-Manager.MinimumScalePercent;float value=span>0?(Manager.Saved.CustomScaleBasisPoints/100-Manager.MinimumScalePercent)/(float)span:1f;if(value<0)value=0;if(value>1)value=1;if(Skin.Slider_SettingValue!=null){Skin.Slider_SettingValue.SetStepSize(span>0?1f/span:1f);Skin.Slider_SettingValue.SetValue(value);}if(Skin.Slider_Bar!=null){var material=Skin.Slider_Bar.GetDynamicMaterial();if(material!=null)material.SetScalarParameterValue("Value",value);}if(Skin.Text_SettingValue!=null)Skin.Text_SettingValue.SetText((Manager.Saved.CustomScaleBasisPoints/100)+"%");}
 void AtPointer(FPointerEvent ev){if(Manager==null || Skin==null || Skin.Slider_SettingValue==null)return;var geo=Skin.Slider_SettingValue.GetCachedGeometry();var size=USlateBlueprintLibrary.GetLocalSize(geo);if(size.X<=0)return;var pos=USlateBlueprintLibrary.AbsoluteToLocal(geo,UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(ev));int percent=Manager.MinimumScalePercent+(int)(pos.X/size.X*(100-Manager.MinimumScalePercent)+0.5);if(percent<Manager.MinimumScalePercent)percent=Manager.MinimumScalePercent;if(percent>100)percent=100;Manager.ChangeScale(percent*100);Refresh();Manager.ShowScaleHelp();}
 protected override UWidget? GetPrimaryGamepadFocusWidget(){return this;}
 public override FEventReply OnPreviewMouseButtonDown(FGeometry geo,FPointerEvent ev){if(Manager!=null)Manager.SelectOwned(Manager.ScaleModel);if(UKismetInputLibrary.PointerEvent_GetEffectingButton(ev).KeyName!="LeftMouseButton")return UWidgetBlueprintLibrary.Unhandled();SetUserFocus(UGameplayStatics.GetPlayerController(this,0));if(Manager!=null)Manager.ShowScaleHelp();if(Skin!=null && Skin.Slider_SettingValue!=null && USlateBlueprintLibrary.IsUnderLocation(Skin.Slider_SettingValue.GetCachedGeometry(),UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(ev))){dragging=true;AtPointer(ev);var reply=UWidgetBlueprintLibrary.Handled();reply=UWidgetBlueprintLibrary.SetUserFocus(ref reply,this,false);return UWidgetBlueprintLibrary.CaptureMouse(ref reply,this);}var focusReply=UWidgetBlueprintLibrary.Handled();return UWidgetBlueprintLibrary.SetUserFocus(ref focusReply,this,false);}
 public override FEventReply OnMouseMove(FGeometry geo,FPointerEvent ev){if(!dragging)return UWidgetBlueprintLibrary.Unhandled();AtPointer(ev);return UWidgetBlueprintLibrary.Handled();}
 public override FEventReply OnMouseButtonDoubleClick(FGeometry geo,FPointerEvent ev){return OnPreviewMouseButtonDown(geo,ev);}
 public override FEventReply OnMouseButtonUp(FGeometry geo,FPointerEvent ev){if(!dragging)return UWidgetBlueprintLibrary.Unhandled();dragging=false;AtPointer(ev);var reply=UWidgetBlueprintLibrary.Handled();return UWidgetBlueprintLibrary.ReleaseMouseCapture(ref reply);}
 public override FEventReply OnPreviewKeyDown(FGeometry geo,FKeyEvent ev){var k=UKismetInputLibrary.GetKey(ev).KeyName;int d=0;if(k=="Left" || k=="Gamepad_DPad_Left")d=-1;if(k=="Right" || k=="Gamepad_DPad_Right")d=1;if(d==0 || Manager==null || Manager.Saved==null)return UWidgetBlueprintLibrary.Unhandled();Manager.ChangeScale(Manager.Saved.CustomScaleBasisPoints+d*100);Refresh();Manager.ShowScaleHelp();return UWidgetBlueprintLibrary.Handled();}
 public override void OnMouseEnter(FGeometry geo,FPointerEvent ev){if(Manager!=null)Manager.ShowScaleHelp();}
 public override FEventReply OnFocusReceived(FGeometry geo,FFocusEvent ev){if(Manager!=null)Manager.ShowScaleHelp();return UWidgetBlueprintLibrary.Handled();}
}

// Separate display/latency intent leaves the released SR schema unchanged.
public class DisplaySettingsSave : USaveGame {
 public int SchemaVersion;public int Revision;public int HDROutput;public int PeakNits;public int PaperWhiteNits;public int UINits;public int ReflexMode;
}
public class DisplayRuntimeSave : USaveGame {
 public int SchemaVersion;public int Revision;public int SessionId;public int ReflexAvailable;public int ReflexMode;public int ReflexFault;public int HDRRevision;public int HDRRestartRequired;
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
   var choices=new List<FText>();if(id==0){choices.Add("Off");choices.Add("On");}else if(id==4){choices.Add("Off");choices.Add("On");choices.Add("On + Boost");}else {int max=id==1?10000:500;int step=id==1?10:1;for(int n=id==1?100:48;n<=max;n+=step)choices.Add(n+" nits");}
   if(Skin.Rotator_SettingValue!=null)Skin.Rotator_SettingValue.PopulateTextLabels(choices);
  }
  int value=Manager.DisplayValue(id);int index=id==1?(value-100)/10:(id==2 || id==3?value-48:value);
  if(Skin.Rotator_SettingValue!=null)Skin.Rotator_SettingValue.SetSelectedItem(index);
  SetIsEnabled(id==0 || (id==4?(Manager.DisplayRuntime!=null && Manager.DisplayRuntime.ReflexAvailable==1 && Manager.DisplayRuntime.ReflexFault==0):Manager.DisplaySaved.HDROutput==1));
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
 public DisplaySettingsSave? DisplaySaved;public DisplayRuntimeSave? DisplayRuntime;public List<DisplaySetting> DisplayModels=new List<DisplaySetting>();int displayAppliedRevision;bool displayHDRSupported;int displayHelp=-1;
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
  InitializeDisplay();Timer.Start(this,"PollDisplay",0.5f,true);
  Timer.Start(this,"PollRenderer",0.25f,true);
  Discover();if(LifecycleWidgets.Count==0)Timer.Start(this,"Discover",0.1f,true);
 }
 public void PollRenderer(){
  RefreshResolutionBounds();
  if(Saved==null || Saved.SchemaVersion!=4 || Saved.Revision<1 || Saved.ReconstructionMode<0 || Saved.ReconstructionMode>2)return;
  var runtime=UGameplayStatics.LoadGameFromSlot("MCD2GraphicsRuntime",0) as GraphicsRuntimeStateSave;
  if(runtime==null || runtime.SchemaVersion!=1 || runtime.RequestedRevision!=Saved.Revision || runtime.SessionId<1)return;bool statusChanged=Runtime==null || Runtime.Phase!=runtime.Phase || Runtime.RequestedRevision!=runtime.RequestedRevision || Runtime.SessionId!=runtime.SessionId;Runtime=runtime;if(statusChanged)ShowHelp();
  // A persisted gameplay flag from another process may never activate menu SR.
  if(Saved.RenderContextSessionId!=runtime.SessionId){Saved.RenderContextSessionId=runtime.SessionId;Saved.Revision++;Persist();return;}
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
 public void RestoreExtensions(){displayHelp=-1;OwnHelpVisible=false;foreach(var p in Panels)if(p.Details_Settings!=null && p.Details_Settings.Box_DetailsExtension!=null)p.Details_Settings.Box_DetailsExtension.SetVisibility(ESlateVisibility.SelfHitTestInvisible);}
 public void SyncLists(){
  if(syncing)return;syncing=true;
  foreach(var p in Panels){
   if(!UKismetSystemLibrary.IsValid(p) || !p.IsVisible() || p.ListView_Settings==null)continue;var r=p.Registry as USpicewoodGameSettingRegistry_PrimaryPlayer;if(r==null || r.GfxSettings==null)continue;
   bool graphics=false;
   foreach(var setting in p.VisibleSettings){var current=setting;int depth=0;while(current!=null && depth<4){if(current==r.GfxSettings)graphics=true;current=current.SettingParent;depth++;}}
   if(!graphics){if(Model!=null && p.ListView_Settings.GetListItems().Contains(Model)){p.ListView_Settings.RemoveItem(Model);if(HeaderModel!=null)p.ListView_Settings.RemoveItem(HeaderModel);if(PresetModel!=null)p.ListView_Settings.RemoveItem(PresetModel);if(ScaleModel!=null)p.ListView_Settings.RemoveItem(ScaleModel);Log.Write("MOD ROWS REMOVED FROM OTHER PAGE");}continue;}
   if(Model==null){
    Probe=UWidgetBlueprintLibrary.Create(this,Unreal.ClassOf<ReconstructionRow>(),UGameplayStatics.GetPlayerController(this,0)) as ReconstructionRow;
    if(Probe==null || Probe.Skin==null || Probe.WidgetTree==null || Probe.WidgetTree.RootWidget==null){Log.Write("PROBE FAILED");continue;}
    Model=UGameplayStatics.SpawnObject(Unreal.ClassOf<ReconstructionSetting>(),r) as ReconstructionSetting;
    if(Model==null)continue;Model.LocalPlayer=r.OwningLocalPlayer;Model.OwningRegistry=r;Model.SettingParent=r.GfxSettings;
    Model.OwnedDisplayName="Upscaler";Model.OwnedHelpText="Native uses the game's temporal anti-aliasing. NVIDIA DLSS uses the preset and render scale below, including DLAA at 100%. Changes apply after a safe history reset.";
   }
   if(HeaderModel==null){HeaderModel=UGameplayStatics.SpawnObject(Unreal.ClassOf<ReconstructionHeaderSetting>(),r) as ReconstructionHeaderSetting;if(HeaderModel!=null){HeaderModel.LocalPlayer=r.OwningLocalPlayer;HeaderModel.OwningRegistry=r;HeaderModel.SettingParent=r.GfxSettings;}}
   if(PresetModel==null){PresetModel=UGameplayStatics.SpawnObject(Unreal.ClassOf<PresetSetting>(),r) as PresetSetting;if(PresetModel!=null){PresetModel.LocalPlayer=r.OwningLocalPlayer;PresetModel.OwningRegistry=r;PresetModel.SettingParent=r.GfxSettings;PresetModel.OwnedDisplayName="DLSS Preset";PresetModel.OwnedHelpText="DLAA: 100%. Quality: about 67%. Balanced: 58%. Performance: 50%. Ultra Performance: about 33%. Custom uses any other scale. Choosing a preset updates the slider and enables NVIDIA DLSS. Presets below the supported minimum are skipped. Quality and Ultra Performance use exact two-thirds and one-third internally.";}}
   if(ScaleModel==null){ScaleModel=UGameplayStatics.SpawnObject(Unreal.ClassOf<ScaleSetting>(),r) as ScaleSetting;if(ScaleModel!=null){ScaleModel.LocalPlayer=r.OwningLocalPlayer;ScaleModel.OwningRegistry=r;ScaleModel.SettingParent=r.GfxSettings;ScaleModel.OwnedDisplayName="Render Scale";ScaleModel.OwnedHelpText="Set internal render resolution in 1% steps. 100% selects DLAA; preset values select the matching preset, and other values select Custom. The minimum follows output resolution (17% at 4K). Display resolution and interface size stay unchanged.";}}
   if(p.ListView_Settings.VisualData==null)continue;var map=p.ListView_Settings.VisualData.EntryWidgetForClass;map[Unreal.ClassOf<ReconstructionHeaderSetting>()]=Unreal.ClassOf<ReconstructionHeaderRow>();map[Unreal.ClassOf<ReconstructionSetting>()]=Unreal.ClassOf<ReconstructionRow>();map[Unreal.ClassOf<PresetSetting>()]=Unreal.ClassOf<PresetRow>();map[Unreal.ClassOf<ScaleSetting>()]=Unreal.ClassOf<ScaleRow>();map[Unreal.ClassOf<DisplaySetting>()]=Unreal.ClassOf<DisplayRow>();p.ListView_Settings.VisualData.EntryWidgetForClass=map;
   InsertDisplayRows(p,r);
   if(HeaderModel!=null && !p.ListView_Settings.GetListItems().Contains(HeaderModel))p.ListView_Settings.AddItem(HeaderModel);
   if(!p.ListView_Settings.GetListItems().Contains(Model)){p.ListView_Settings.AddItem(Model);Log.Write("RECONSTRUCTION APPENDED FROM EVENT");}
   if(PresetModel!=null && !p.ListView_Settings.GetListItems().Contains(PresetModel))p.ListView_Settings.AddItem(PresetModel);if(ScaleModel!=null && !p.ListView_Settings.GetListItems().Contains(ScaleModel))p.ListView_Settings.AddItem(ScaleModel);
  }
  syncing=false;
 }
 public void SelectOwned(UObject? item){if(item==null)return;PendingFocus=item;foreach(var p in Panels)if(p.ListView_Settings!=null && p.ListView_Settings.GetListItems().Contains(item))p.ListView_Settings.BP_NavigateToItem(item);Timer.Start(this,"RestoreOwnedFocus",0.1f,false);}
 public void RestoreOwnedFocus(){var item=PendingFocus;PendingFocus=null;if(item==null)return;foreach(var p in Panels){if(p.ListView_Settings==null || !p.IsVisible() || !p.ListView_Settings.GetListItems().Contains(item))continue;p.ListView_Settings.BP_NavigateToItem(item);foreach(var row in p.ListView_Settings.GetDisplayedEntryWidgets()){if((item==Model && row is ReconstructionRow) || (item==PresetModel && row is PresetRow) || (item==ScaleModel && row is ScaleRow) || (item is DisplaySetting && row is DisplayRow && (row as DisplayRow).Setting==item))row.SetUserFocus(UGameplayStatics.GetPlayerController(this,0));}}}
 public void ShowReconstructionHelp(){displayHelp=-1;HelpKind=0;OwnHelpVisible=true;ShowHelp();}
 public void ShowPresetHelp(){displayHelp=-1;HelpKind=1;OwnHelpVisible=true;ShowHelp();}
 public void ShowScaleHelp(){displayHelp=-1;HelpKind=2;OwnHelpVisible=true;ShowHelp();}
 public void ShowHelp(){
  if(displayHelp>=0){ShowDisplayHelp(displayHelp);return;}
  if(Model==null || !OwnHelpVisible)return;string title="Upscaler";string help="Native uses the game temporal anti-aliasing. NVIDIA DLSS uses the preset and render scale below, including DLAA at 100%. Resume gameplay to apply changes after a safe history reset.";if(HelpKind==1 && PresetModel!=null){title="DLSS Preset";help="DLAA: 100%. Quality: about 67%. Balanced: 58%. Performance: 50%. Ultra Performance: about 33%. Custom uses any other scale. Choosing a preset updates the slider and enables NVIDIA DLSS. Presets below the supported minimum are skipped. Quality and Ultra Performance use exact two-thirds and one-third internally.";}if(HelpKind==2 && ScaleModel!=null){title="Render Scale";help="Set internal render resolution in 1% steps. 100% selects DLAA; preset values select the matching preset, and other values select Custom. The minimum follows output resolution (17% at 4K). Display resolution and interface size stay unchanged.";}
  if(HelpKind==2)help+="\nCurrent minimum: "+MinimumScalePercent+"%.";if(outputWidth>3840 || outputHeight>2160 || (outputWidth>0 && !ScaleSupported(10000)))help+="\nDLSS is unavailable at this output resolution. Supported output: 640 x 360 to 3840 x 2160.";
  foreach(var p in Panels){if(p.Details_Settings==null || p.ListView_Settings==null || !p.ListView_Settings.GetListItems().Contains(Model))continue;
   var d=p.Details_Settings;if(d.Text_SettingName!=null)d.Text_SettingName.SetText(title);
   
   if(d.RichText_DynamicDetails!=null){string status="Saved. Resume gameplay to apply safely.";if(Runtime!=null && Saved!=null && Runtime.RequestedRevision==Saved.Revision){if(Runtime.Phase==1 || Runtime.Phase==4)status="Applying upscaler change...";if(Runtime.Phase==2)status=Saved.RenderContextReady==0 && Saved.ReconstructionMode!=0?"DLSS selection saved. It activates in gameplay.":"Upscaler setting is active.";if(Runtime.Phase==3)status=Runtime.ErrorCode==2?"This render scale is unsupported; native anti-aliasing is active.":"DLSS unavailable; native anti-aliasing is active.";}d.RichText_DynamicDetails.SetText(HelpKind==0?"Default: Native.":(HelpKind==1?"Default: Quality.":"Default: 67%."));if(d.RichText_Description!=null)d.RichText_Description.SetText(help+"\n\n"+status);}if(d.RichText_WarningDetails!=null)d.RichText_WarningDetails.SetText("");if(d.RichText_DisabledDetails!=null)d.RichText_DisabledDetails.SetText("");
   if(d.Box_DetailsExtension!=null)d.Box_DetailsExtension.SetVisibility(ESlateVisibility.Collapsed);
  }
 }
 // Preset IDs 0-4 retain their saved meanings; DLAA is appended as 5.
 public int PresetSliderScale(int preset){if(preset==5)return 10000;if(preset==0)return 6700;if(preset==1)return 5800;if(preset==2)return 5000;if(preset==3)return 3300;return Saved==null?6700:Saved.CustomScaleBasisPoints;}
 public int PresetAtScale(int scale){if(scale==10000)return 5;if(scale==6700)return 0;if(scale==5800)return 1;if(scale==5000)return 2;if(scale==3300)return 3;return 4;}
 // Use output viewport dimensions, never the reduced source dimensions.
 public bool ScaleSupported(int scale){
  if(outputWidth<=0 || outputHeight<=0)return scale>=1700;
  if(outputWidth>3840 || outputHeight>2160)return false;
  int divisor=10000;int numerator=scale;
  if(scale==6700){divisor=3;numerator=2;}if(scale==3300){divisor=3;numerator=1;}
  return (long)outputWidth*numerator>=640L*divisor && (long)outputHeight*numerator>=360L*divisor;
 }
 public int ClampScale(int scale){if(scale<MinimumScalePercent*100)scale=MinimumScalePercent*100;if(scale>10000)scale=10000;return scale;}
 public void RefreshResolutionBounds(){
  var player=UGameplayStatics.GetPlayerController(this,0);if(player==null)return;
  player.GetViewportSize(out int width,out int height);if(width<=0 || height<=0 || (width==outputWidth && height==outputHeight))return;
  outputWidth=width;outputHeight=height;MinimumScalePercent=100;
  for(int percent=1;percent<=100;percent++){if(ScaleSupported(percent*100)){MinimumScalePercent=percent;break;}}
  if(Saved!=null && Saved.SchemaVersion==4){
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
  if(Saved==null || Saved.SchemaVersion!=4 || direction==0)return;
  if(!ScaleSupported(10000))return;
  Saved.ReconstructionMode=Saved.ReconstructionMode==0?(Saved.SRPreset==5?1:2):0;UpdateScale();Saved.Revision++;Persist();RefreshRows();
 }
 public void UpdateScale(){if(Saved==null)return;int scale=10000;if(Saved.ReconstructionMode==2){scale=6667;if(Saved.SRPreset==1)scale=5800;if(Saved.SRPreset==2)scale=5000;if(Saved.SRPreset==3)scale=3333;if(Saved.SRPreset==4)scale=Saved.CustomScaleBasisPoints;}Saved.RenderScaleBasisPoints=scale;}
 public void ChangePreset(int direction){if(Saved==null || Saved.SchemaVersion!=4)return;int index=(Saved.SRPreset==5?0:Saved.SRPreset+1)+direction;if(index<0)index=5;if(index>5)index=0;int p=index==0?5:index-1;for(int attempt=0;attempt<6;attempt++){int candidate=p==4?ClampScale(Saved.LastCustomScaleBasisPoints):PresetSliderScale(p);if(ScaleSupported(candidate))break;index+=direction;if(index<0)index=5;if(index>5)index=0;p=index==0?5:index-1;}if(!ScaleSupported(10000))return;Saved.CustomScaleBasisPoints=p==4?ClampScale(Saved.LastCustomScaleBasisPoints):PresetSliderScale(p);p=PresetAtScale(Saved.CustomScaleBasisPoints);Saved.SRPreset=p;Saved.ReconstructionMode=p==5?1:2;UpdateScale();Saved.Revision++;Persist();RefreshRows();}
 public void ChangeScale(int scale){if(Saved==null || Saved.SchemaVersion!=4)return;if(!ScaleSupported(10000))return;scale=ClampScale(scale);int p=PresetAtScale(scale);int mode=p==5?1:2;if(Saved.CustomScaleBasisPoints==scale && Saved.ReconstructionMode==mode && Saved.SRPreset==p)return;Saved.CustomScaleBasisPoints=scale;if(p==4)Saved.LastCustomScaleBasisPoints=scale;Saved.SRPreset=p;Saved.ReconstructionMode=mode;UpdateScale();Saved.Revision++;Persist();RefreshRows();}
 public void ResetOwned(){if(Saved==null || Saved.SchemaVersion!=4)return;Saved.ReconstructionMode=0;Saved.SRPreset=0;Saved.CustomScaleBasisPoints=6700;Saved.LastCustomScaleBasisPoints=7700;Saved.RenderScaleBasisPoints=10000;Saved.NativeFallback=true;Saved.Revision++;Persist();if(DisplaySaved!=null){DisplaySaved.HDROutput=0;DisplaySaved.PeakNits=1000;DisplaySaved.PaperWhiteNits=203;DisplaySaved.UINits=203;DisplaySaved.ReflexMode=1;DisplaySaved.Revision++;PersistDisplay();}RefreshRows();Log.Write("OWNED RESET");}
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
 public int DisplayValue(int id){if(DisplaySaved==null)return 0;if(id==0)return DisplaySaved.HDROutput;if(id==1)return DisplaySaved.PeakNits;if(id==2)return DisplaySaved.PaperWhiteNits;if(id==3)return DisplaySaved.UINits;return DisplaySaved.ReflexMode;}
 public string DisplayLabel(int id){if(id==0)return "HDR Output";if(id==1)return "HDR Peak Brightness";if(id==2)return "HDR Paper White";if(id==3)return "HDR UI Brightness";return "NVIDIA Reflex";}
 public void ChangeDisplay(int id,int direction){if(DisplaySaved==null || DisplaySaved.SchemaVersion!=1 || direction==0)return;
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
  if(supportChanged)SyncLists();
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
  if(DisplayModels.Count==0){for(int id=0;id<5;id++){var model=UGameplayStatics.SpawnObject(Unreal.ClassOf<DisplaySetting>(),registry) as DisplaySetting;if(model!=null){model.ControlId=id;model.LocalPlayer=registry.OwningLocalPlayer;model.OwningRegistry=registry;model.SettingParent=registry.GfxSettings;DisplayModels.Add(model);}}}
  var list=panel.ListView_Settings.GetListItems();var result=new List<UObject>();bool hdrInserted=false;bool reflexInserted=false;
  foreach(var item in list){if(item is DisplaySetting)continue;result.Add(item);if(item==Model || item==HeaderModel || item==PresetModel || item==ScaleModel)continue;var setting=item as UGameSetting;if(setting==null)continue;string label=UKismetTextLibrary.Conv_TextToString(setting.GetDisplayName());string name=UKismetSystemLibrary.GetObjectName(setting);
   if(!hdrInserted && (label=="Brightness" || name.Contains("Brightness"))){foreach(var model in DisplayModels)if(model.ControlId<4)result.Add(model);hdrInserted=true;}
   if(!reflexInserted && (label=="FPS Limit" || label.Contains("Frame Rate Limit") || name.Contains("FrameRateLimit") || name.Contains("FPSLimit"))){foreach(var model in DisplayModels)if(model.ControlId==4 && ((DisplayRuntime!=null && DisplayRuntime.ReflexAvailable==1 && DisplayRuntime.ReflexFault==0) || list.Contains(model)))result.Add(model);reflexInserted=true;}
  }
  // Fail closed when the native anchor cannot be identified; no misplaced giant mod block.
  bool changed=list.Count!=result.Count;if(!changed){for(int i=0;i<list.Count;i++)if(list[i]!=result[i])changed=true;}
  if(changed)panel.ListView_Settings.BP_SetListItems(result);RefreshDisplayRows();
 }
 public void RefreshDisplayRows(){UWidgetBlueprintLibrary.GetAllWidgetsOfClass(this,out List<UUserWidget> rows,Unreal.ClassOf<DisplayRow>(),false);foreach(var w in rows){var row=w as DisplayRow;if(row!=null && UKismetSystemLibrary.IsValid(row) && row.IsVisible())row.Refresh();}}
 public void ShowDisplayHelp(int id){displayHelp=id;OwnHelpVisible=true;string help="";
  if(id==0)help="Enables the game's HDR output path and RenoDX HDR processing. System HDR must also be enabled.";
  if(id==1)help="Set to the approximate peak luminance of the display's HDR mode. Use your display's specification or measured clipping point.";
  if(id==2)help="Controls the brightness of ordinary scene white. Default: 203 nits.";
  if(id==3)help="Controls interface brightness in HDR. Default: 203 nits.";
  if(id==4)help="Reduces the render queue using NVIDIA Reflex. On + Boost also requests higher GPU clocks. Off retains measurement markers. Default: On.";
  if(id==4 && (DisplayRuntime==null || DisplayRuntime.ReflexAvailable!=1 || DisplayRuntime.ReflexFault!=0))help+="\n\nReflex is unavailable in this session. Your saved choice is retained. Restart the game and check the installation if this persists.";
  if(id<4){if(!displayHDRSupported)help+="\n\nHDR output is unavailable. Enable system HDR and check your display/Proton setup.";if(DisplayRuntime!=null && DisplayRuntime.HDRRestartRequired==1)help+="\n\nRestart the game to apply RenoDX calibration. Advanced controls remain in the ReShade panel.";}
  foreach(var p in Panels){if(!UKismetSystemLibrary.IsValid(p) || !p.IsVisible() || p.Details_Settings==null || p.ListView_Settings==null || Model==null || !p.ListView_Settings.GetListItems().Contains(Model))continue;var d=p.Details_Settings;if(d.Text_SettingName!=null)d.Text_SettingName.SetText(DisplayLabel(id));if(d.RichText_Description!=null)d.RichText_Description.SetText(help);if(d.RichText_DynamicDetails!=null)d.RichText_DynamicDetails.SetText(id==4 && DisplayRuntime!=null?"Active mode: "+(DisplayRuntime.ReflexMode==0?"Off":DisplayRuntime.ReflexMode==1?"On":"On + Boost"):"");if(d.RichText_WarningDetails!=null)d.RichText_WarningDetails.SetText("");if(d.RichText_DisabledDetails!=null)d.RichText_DisabledDetails.SetText("");if(d.Box_DetailsExtension!=null)d.Box_DetailsExtension.SetVisibility(ESlateVisibility.Collapsed);}
 }

 protected override void ReceiveEndPlay(EEndPlayReason reason){
  if(Saved!=null && Saved.SchemaVersion==4 && Saved.RenderContextReady!=0){Saved.RenderContextReady=0;Saved.RenderContextSessionId=0;Saved.Revision++;Persist();}
  Timer.Stop(this,"Discover");Timer.Stop(this,"SyncLists");Timer.Stop(this,"PollRenderer");Timer.Stop(this,"PollDisplay");Timer.Stop(this,"RestoreOwnedFocus");foreach(var button in ResetButtons)button.OnButtonBaseClicked-=ResetConfirmed;foreach(var active in LifecycleWidgets){active.BP_OnWidgetActivated-=LifecycleChanged;active.BP_OnWidgetDeactivated-=LifecycleChanged;}foreach(var p in Panels){if(p.ListView_Settings==null)continue;p.ListView_Settings.BP_OnEntriesGenerated-=EntriesGenerated;p.ListView_Settings.BP_OnEntryReleased-=EntryReleased;p.ListView_Settings.BP_OnItemSelectionChanged-=SelectionChanged;p.ListView_Settings.BP_OnItemIsHoveredChanged-=HoverChanged;if(Model!=null)p.ListView_Settings.RemoveItem(Model);if(HeaderModel!=null)p.ListView_Settings.RemoveItem(HeaderModel);if(PresetModel!=null)p.ListView_Settings.RemoveItem(PresetModel);if(ScaleModel!=null)p.ListView_Settings.RemoveItem(ScaleModel);foreach(var ds in DisplayModels)p.ListView_Settings.RemoveItem(ds);}Log.Write("LIST EVENTS UNBOUND");
 }
}
