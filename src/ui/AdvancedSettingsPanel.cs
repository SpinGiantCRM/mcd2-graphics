using NeoRune;
using MCD2Graphics;
using System.Collections.Generic;
using UE.CoreUObject;
using UE.Engine;
using UE.UMG;
using UE.SlateCore;
using UE.Angelscript;

// Embedded in Blueprint Loader's Mods page. Each category has a bounded list;
// help stays visible beside it rather than adding a paragraph below every row.
public class AdvancedSettingsPanel : ModWidget, IModResettable {
 public ModActor? Manager;public UVerticalBox? Controls;public UTextBlock? HelpTitle;public UTextBlock? HelpBody;public UTextBlock? Footer;
 public int Category;public int Selected;public List<AdvancedSettingRow> Rows=new List<AdvancedSettingRow>();
 public AdvancedInfoWidget? Details;
 protected override UWidget? Build(UWidgetTree tree){
  var column=Ui.Column(tree);if(column==null)return null;
  var tabs=Ui.Row(tree);if(tabs!=null){var up=WithSounds(Ui.Button(tree,"UPSCALING",22));var fg=WithSounds(Ui.Button(tree,"FRAME GENERATION",22));var diag=WithSounds(Ui.Button(tree,"DIAGNOSTICS",22));
   if(up!=null){up.OnClicked+=Upscaling;tabs.AddChild(up);}if(fg!=null){fg.OnClicked+=FrameGeneration;tabs.AddChild(fg);}if(diag!=null){diag.OnClicked+=Diagnostics;}column.AddChild(tabs);}
  var body=Ui.Row(tree);Controls=Ui.Column(tree);var help=Ui.Column(tree);
  HelpTitle=Ui.Text(tree,"ADVANCED GRAPHICS",26);HelpBody=Ui.Text(tree,"Select a setting to see its description.",22);Footer=Ui.Text(tree,"",22);
  if(HelpBody!=null)HelpBody.SetAutoWrapText(true);if(Footer!=null)Footer.SetAutoWrapText(true);
  if(help!=null){if(HelpTitle!=null)help.AddChild(HelpTitle);if(HelpBody!=null)help.AddChild(HelpBody);if(Footer!=null)help.AddChild(Footer);}
  if(body!=null){if(Controls!=null){var slot=body.AddChildToHorizontalBox(Controls);if(slot!=null){slot.SetSize(new FSlateChildSize{SizeRule=ESlateSizeRule.Fill,Value=0.60f});slot.SetPadding(Ui.Margin(10));}}
   var panel=Ui.Panel(tree,help,Ui.Color(0.02f,0.06f,0.08f,0.70f),24);if(panel!=null){var slot=body.AddChildToHorizontalBox(panel);if(slot!=null){slot.SetSize(new FSlateChildSize{SizeRule=ESlateSizeRule.Fill,Value=0.40f});slot.SetPadding(Ui.Margin(10));}}
   var size=UGameplayStatics.SpawnObject(Unreal.ClassOf<USizeBox>(),tree) as USizeBox;if(size!=null){size.SetMinDesiredHeight(470);size.AddChild(body);column.AddChild(size);}else column.AddChild(body);}
  return column;
 }
 public override void Construct(){UGameplayStatics.GetAllActorsOfClass(this,Unreal.ClassOf<ModActor>(),out List<AActor> actors);if(actors.Count>0)Manager=actors[0] as ModActor;Populate();Timer.Start(this,"Refresh",0.5f,true);}
 public override void Destruct(){Timer.Stop(this,"Refresh");Rows.Clear();Manager=null;}
 public void Upscaling(){Category=0;Populate();}public void FrameGeneration(){Category=1;Populate();}public void Diagnostics(){Category=2;Populate();}
 public void Populate(){
  if(Controls==null||Manager==null)return;Controls.ClearChildren();Rows.Clear();Details=null;
  if(Category==0){Add(0);Add(1);}else if(Category==1){Add(2);Add(3);}else{Add(4);Add(5);Details=Ui.Create(this,Unreal.ClassOf<AdvancedInfoWidget>()) as AdvancedInfoWidget;if(Details!=null){var scroll=UGameplayStatics.SpawnObject(Unreal.ClassOf<UScrollBox>(),WidgetTree) as UScrollBox;if(scroll!=null){scroll.AddChild(Details);var box=UGameplayStatics.SpawnObject(Unreal.ClassOf<USizeBox>(),WidgetTree) as USizeBox;if(box!=null){box.SetHeightOverride(300);box.AddChild(scroll);Controls.AddChild(box);}}}}
  Selected=Category==0?0:Category==1?2:4;ShowHelp(Selected);Refresh();
 }
 void Add(int setting){if(Controls==null)return;var row=Ui.Create(this,Unreal.ClassOf<AdvancedSettingRow>()) as AdvancedSettingRow;if(row==null)return;row.Panel=this;row.Setting=setting;Rows.Add(row);Controls.AddChild(row);row.Refresh();}
 public void Refresh(){foreach(var row in Rows)row.Refresh();}
 public void FocusRow(int setting,int direction){for(int i=0;i<Rows.Count;i++)if(Rows[i].Setting==setting){int target=i+direction;if(target<0)target=Rows.Count-1;if(target>=Rows.Count)target=0;Rows[target].SetUserFocus(World.PlayerController(this));ShowHelp(Rows[target].Setting);return;}}
 public string Title(int setting){if(setting==0)return "DLSS Model Preset";if(setting==1)return "FSR Upscaling Version";if(setting==2)return "DLSS FG Preset";if(setting==3)return "FSR FG Version";if(setting==4)return "Show Runtime Details";return "Show In-Game Diagnostics";}
 public List<FText> Choices(int setting){
  if(setting==0)return new List<FText>{"Auto","E - CNN (Legacy)","F - CNN (Legacy)","J - Transformer","K - Transformer","L - Transformer 2nd Gen","M - Transformer 2nd Gen"};
  if(setting==1)return new List<FText>{"Auto","3.1 - Analytical","4.1 - INT8 (Experimental)"};
  if(setting==2)return new List<FText>{"Auto","A - Standard","B - UI Recomposition","C - Performance"};
  if(setting==3)return new List<FText>{"Auto","3.1 - Analytical","4.0 - ML (Experimental)"};return new List<FText>{"Off","On"};
 }
 public int Value(int setting){if(Manager==null)return 0;if(setting==0)return Manager.Advanced==null?0:AdvancedMenuClient.PresetChoice(Manager.Advanced.Hint);if(setting==1)return Manager.AdvancedFSR==null?0:Manager.AdvancedFSR.Hint;if(setting==2)return Manager.AdvancedFG==null?0:Manager.AdvancedFG.Hint;if(setting==3)return Manager.AdvancedFSRFG==null?0:Manager.AdvancedFSRFG.Hint;if(setting==4)return Manager.ShowMenuDiagnostics?1:0;return Manager.ShowDiagnostics?1:0;}
 public void Change(int setting,int direction){
  if(Manager==null)return;int count=Choices(setting).Count;int value=Value(setting)+direction;if(value<0)value=count-1;if(value>=count)value=0;
  string key=setting==0?"dlss_sr_preset":setting==1?"fsr_sr_version":setting==2?"dlss_fg_preset":setting==3?"fsr_fg_version":setting==4?"show_runtime_details":"show_runtime_info";
  if(setting<4){ModSettings.SetInt(Manager,key,value);Manager.OnSettingChanged(key,""+value);}else{ModSettings.SetBool(Manager,key,value==1);Manager.OnSettingChanged(key,value==1?"true":"false");}
  ShowHelp(setting);Refresh();Sounds.Click(this);
 }
 public void ShowHelp(int setting){
  Selected=setting;if(HelpTitle!=null)HelpTitle.SetText(Title(setting).ToUpper());if(HelpBody==null)return;
  string body="";string footer="Recommended: AUTO";
  if(setting==0)body="Auto follows the DLSS quality mode.\n\nE / F: legacy CNN\nJ / K: Transformer\nL / M: second-generation Transformer\n\nAn explicit preset overrides Auto. Restart after changes. The driver may supersede your choice.";
  if(setting==1)body="Choose the FSR upscaling implementation.\n\n3.1: analytical upscaling.\n4.1 INT8: experimental community build for a wider range of GPUs.\n\nAuto currently uses 3.1. An explicit 4.1 selection never silently falls back to 3.1. Restart after changes.";
  if(setting==2)body="Choose the DLSS frame-generation model.\n\nA: standard path.\nB: separate scene and UI interpolation.\nC: lighter optical-flow path.\n\nRestart after changes. C uses a driver override; see Diagnostics for whether the request was accepted.";
  if(setting==3)body="Choose the FSR frame-generation implementation independently of upscaling.\n\n3.1: analytical frame generation.\n4.0 ML: requires a supported GPU and driver.\n\nPortable FSR 4.1 upscaling does not add portable ML frame generation. Auto uses 3.1. Restart after changes.";
  if(setting==4){body="Show runtime library versions, requested presets and the last verified FSR implementations in this Diagnostics section.\n\nA loaded library or saved setting does not prove a model is active.";footer="Recommended: OFF";}
  if(setting==5){body="Show runtime activity while playing.\n\nReports advancing upscaler, generated-frame and low-latency counters. Stale or suspended activity is shown as waiting.\n\nReflex and AMD Anti-Lag 2 remain independently selectable in Video when supported.";footer="Recommended: OFF";}
  HelpBody.SetText("\n"+body+"\n");if(Footer!=null)Footer.SetText("\n"+footer);
 }
}
public class AdvancedSettingRow : ModWidget {
 public AdvancedSettingsPanel? Panel;public int Setting;public UAS_SettingsEntry_Stepper? Skin;
 protected override UWidget? Build(UWidgetTree tree){
  bIsFocusable=true;Skin=Ui.GameWidget(this,"/SpicewoodSettings/Spicewood/UI/SettingsScreen/Editors/W_SettingsEntry_Stepper.W_SettingsEntry_Stepper_C") as UAS_SettingsEntry_Stepper;
  if(Skin==null||Skin.WidgetTree==null)return null;
  if(Skin.Button_Decrease!=null){Skin.Button_Decrease.SetVisibility(ESlateVisibility.HitTestInvisible);Skin.Button_Decrease.SetIsFocusable(false);}if(Skin.Button_Increase!=null){Skin.Button_Increase.SetVisibility(ESlateVisibility.HitTestInvisible);Skin.Button_Increase.SetIsFocusable(false);}if(Skin.Rotator_SettingValue!=null)Skin.Rotator_SettingValue.SetVisibility(ESlateVisibility.HitTestInvisible);
  return Skin.WidgetTree.RootWidget;
 }
 public void Refresh(){if(Skin==null||Panel==null)return;if(Skin.Text_SettingName!=null)Skin.Text_SettingName.SetText(Panel.Title(Setting));if(Skin.Rotator_SettingValue!=null){Skin.Rotator_SettingValue.PopulateTextLabels(Panel.Choices(Setting));Skin.Rotator_SettingValue.SetSelectedItem(Panel.Value(Setting));}}
 public override void OnMouseEnter(FGeometry g,FPointerEvent e){if(Panel!=null)Panel.ShowHelp(Setting);}
 public override FEventReply OnFocusReceived(FGeometry g,FFocusEvent e){if(Panel!=null)Panel.ShowHelp(Setting);return UWidgetBlueprintLibrary.Handled();}
 public override FEventReply OnPreviewKeyDown(FGeometry g,FKeyEvent e){if(Panel==null)return UWidgetBlueprintLibrary.Unhandled();var key=UKismetInputLibrary.GetKey(e).KeyName;if(key=="Left"||key=="Gamepad_DPad_Left"){Panel.Change(Setting,-1);return UWidgetBlueprintLibrary.Handled();}if(key=="Right"||key=="Enter"||key=="SpaceBar"||key=="Gamepad_DPad_Right"||key=="Gamepad_FaceButton_Bottom"){Panel.Change(Setting,1);return UWidgetBlueprintLibrary.Handled();}if(key=="Up"||key=="Down"||key=="Gamepad_DPad_Up"||key=="Gamepad_DPad_Down"){Panel.FocusRow(Setting,key=="Up"||key=="Gamepad_DPad_Up"?-1:1);return UWidgetBlueprintLibrary.Handled();}return UWidgetBlueprintLibrary.Unhandled();}
 public override FEventReply OnPreviewMouseButtonDown(FGeometry g,FPointerEvent e){if(Panel==null||UKismetInputLibrary.PointerEvent_GetEffectingButton(e).KeyName!="LeftMouseButton")return UWidgetBlueprintLibrary.Unhandled();SetUserFocus(World.PlayerController(this));Panel.ShowHelp(Setting);var position=UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(e);if(Skin!=null&&Skin.Button_Decrease!=null&&USlateBlueprintLibrary.IsUnderLocation(Skin.Button_Decrease.GetCachedGeometry(),position))Panel.Change(Setting,-1);else if(Skin!=null&&Skin.Button_Increase!=null&&USlateBlueprintLibrary.IsUnderLocation(Skin.Button_Increase.GetCachedGeometry(),position))Panel.Change(Setting,1);return UWidgetBlueprintLibrary.Handled();}
}
