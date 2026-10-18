using NeoRune;
using UE.Engine;
using UE.CoreUObject;
using UE.UMG;
using UE.GameSettings;
using UE.Angelscript;
using UE.SlateCore;
using System.Collections.Generic;
namespace MCD2Graphics;
public class SharpnessSetting : UGameSetting {}
public class SharpnessRow : UGameSettingListEntryBase {
 public UAS_SettingsEntry_Scalar? Skin;public ModActor? Manager;bool dragging;
 public override void OnInitialized(){
  UGameplayStatics.GetAllActorsOfClass(this,Unreal.ClassOf<ModActor>(),out List<AActor> actors);if(actors.Count>0)Manager=actors[0] as ModActor;
  Skin=UWidgetBlueprintLibrary.Create(this,Unreal.ClassAt<UAS_SettingsEntry_Scalar>("/SpicewoodSettings/Spicewood/UI/SettingsScreen/Editors/W_SettingsEntry_Scalar.W_SettingsEntry_Scalar_C"),UGameplayStatics.GetPlayerController(this,0)) as UAS_SettingsEntry_Scalar;
  if(Skin==null || Skin.WidgetTree==null || Skin.WidgetTree.RootWidget==null || WidgetTree==null)return;WidgetTree.RootWidget=Skin.WidgetTree.RootWidget;bIsFocusable=true;
  Skin.WidgetTree.RootWidget.SetVisibility(ESlateVisibility.Visible);
  if(Skin.Text_SettingName!=null)Skin.Text_SettingName.SetText("Upscaling Sharpness");
  if(Skin.Slider_SettingValue!=null){Skin.Slider_SettingValue.SetMinValue(0);Skin.Slider_SettingValue.SetMaxValue(1);Skin.Slider_SettingValue.SetStepSize(0.01f);Skin.Slider_SettingValue.SetLocked(true);}Refresh();
 }
 public override void Construct(){Refresh();}
 public void Refresh(){if(Skin==null||Manager==null)return;float value=Manager.Sharpness/100f;
  SetIsEnabled(Manager.SelectedProvider()!=0);
  if(Skin.Slider_SettingValue!=null){Skin.Slider_SettingValue.SetStepSize(0.01f);Skin.Slider_SettingValue.SetValue(value);}
  if(Skin.Slider_Bar!=null){var material=Skin.Slider_Bar.GetDynamicMaterial();if(material!=null)material.SetScalarParameterValue("Value",value);}
  if(Skin.Text_SettingValue!=null)Skin.Text_SettingValue.SetText(Manager.Sharpness+"%");
 }
 void AtPointer(FPointerEvent ev){if(Manager==null || Skin==null || Skin.Slider_SettingValue==null)return;var geo=Skin.Slider_SettingValue.GetCachedGeometry();var size=USlateBlueprintLibrary.GetLocalSize(geo);if(size.X<=0)return;var pos=USlateBlueprintLibrary.AbsoluteToLocal(geo,UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(ev));int percent=(int)(pos.X/size.X*100+0.5);if(percent<0)percent=0;if(percent>100)percent=100;Manager.ChangeSharpness(percent);Refresh();Manager.ShowSharpnessHelp();}
 protected override UWidget? GetPrimaryGamepadFocusWidget(){return this;}
 public override FEventReply OnPreviewMouseButtonDown(FGeometry geo,FPointerEvent ev){if(Manager!=null)Manager.SelectOwned(Manager.SharpnessModel);if(UKismetInputLibrary.PointerEvent_GetEffectingButton(ev).KeyName!="LeftMouseButton")return UWidgetBlueprintLibrary.Unhandled();SetUserFocus(UGameplayStatics.GetPlayerController(this,0));if(Manager!=null)Manager.ShowSharpnessHelp();if(Skin!=null && Skin.Slider_SettingValue!=null && USlateBlueprintLibrary.IsUnderLocation(Skin.Slider_SettingValue.GetCachedGeometry(),UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(ev))){dragging=true;AtPointer(ev);var reply=UWidgetBlueprintLibrary.Handled();reply=UWidgetBlueprintLibrary.SetUserFocus(ref reply,this,false);return UWidgetBlueprintLibrary.CaptureMouse(ref reply,this);}var focusReply=UWidgetBlueprintLibrary.Handled();return UWidgetBlueprintLibrary.SetUserFocus(ref focusReply,this,false);}
 public override FEventReply OnMouseMove(FGeometry geo,FPointerEvent ev){if(!dragging)return UWidgetBlueprintLibrary.Unhandled();AtPointer(ev);return UWidgetBlueprintLibrary.Handled();}
 public override FEventReply OnMouseButtonDoubleClick(FGeometry geo,FPointerEvent ev){return OnPreviewMouseButtonDown(geo,ev);}
 public override FEventReply OnMouseButtonUp(FGeometry geo,FPointerEvent ev){if(!dragging)return UWidgetBlueprintLibrary.Unhandled();dragging=false;AtPointer(ev);var reply=UWidgetBlueprintLibrary.Handled();return UWidgetBlueprintLibrary.ReleaseMouseCapture(ref reply);}
 public override FEventReply OnPreviewKeyDown(FGeometry geo,FKeyEvent ev){var k=UKismetInputLibrary.GetKey(ev).KeyName;int d=0;if(k=="Left" || k=="Gamepad_DPad_Left")d=-1;if(k=="Right" || k=="Gamepad_DPad_Right")d=1;if(d==0 || Manager==null || Manager.Saved==null)return UWidgetBlueprintLibrary.Unhandled();Manager.ChangeSharpness(Manager.Sharpness+d);Refresh();Manager.ShowSharpnessHelp();return UWidgetBlueprintLibrary.Handled();}
 public override void OnMouseEnter(FGeometry geo,FPointerEvent ev){if(Manager!=null)Manager.ShowSharpnessHelp();}
 public override FEventReply OnFocusReceived(FGeometry geo,FFocusEvent ev){if(Manager!=null)Manager.ShowSharpnessHelp();return UWidgetBlueprintLibrary.Handled();}
}

