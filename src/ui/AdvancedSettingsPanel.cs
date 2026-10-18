using NeoRune;
using MCD2Graphics;
using System.Collections.Generic;
using UE.CoreUObject;
using UE.Engine;
using UE.UMG;
using UE.SlateCore;
using UE.Angelscript;
using UE.GameSettings;
using UE.CommonUI;

// Embedded in Blueprint Loader's Mods page. Each category has a bounded list;
// help stays visible beside it rather than adding a paragraph below every row.
public class AdvancedSettingsPanel : ModWidget, IModResettable {
 public ModActor? Manager;public UVerticalBox? Controls;
 public int Category;public int Selected;public List<AdvancedSettingRow> Rows=new List<AdvancedSettingRow>();
 public UE.SpicewoodSettings.Spicewood.UI.SettingsScreen.W_SettingsPanel.UW_SettingsPanel_C? HelpShell;
 public AdvancedInfoWidget? Details;public UGameSettingDetailView? NativeHelp;public List<UAS_SpicewoodButtonTabAction> Tabs=new List<UAS_SpicewoodButtonTabAction>();
 protected override UWidget? Build(UWidgetTree tree){
  var column=Ui.Column(tree);if(column==null)return null;
  var left=Ui.Column(tree);Controls=Ui.Column(tree);var tabs=Ui.Row(tree);
  if(tabs!=null){AddTab(tabs,"UPSCALING");AddTab(tabs,"FRAME GENERATION");AddTab(tabs,"DIAGNOSTICS");
   var bar=Ui.Panel(tree,tabs,Ui.Color(0.02f,0.015f,0.01f,1),0);if(left!=null&&bar!=null)left.AddChild(bar);}
  if(left!=null&&Controls!=null)left.AddChild(Controls);
  HelpShell=Ui.GameWidget(this,"/SpicewoodSettings/Spicewood/UI/SettingsScreen/W_SettingsPanel.W_SettingsPanel_C") as UE.SpicewoodSettings.Spicewood.UI.SettingsScreen.W_SettingsPanel.UW_SettingsPanel_C;
  NativeHelp=HelpShell==null?NativeSettingsWidgets.HelpCard(this):HelpShell.Details_Settings;
  if(NativeHelp==null)NativeHelp=NativeSettingsWidgets.HelpCard(this);
  var help=NativeSettingsWidgets.HelpFrame(tree,HelpShell,NativeHelp);var body=Ui.Row(tree);
  if(body!=null){if(left!=null){var slot=body.AddChildToHorizontalBox(left);if(slot!=null){slot.SetSize(new FSlateChildSize{SizeRule=ESlateSizeRule.Fill,Value=2});slot.SetPadding(Ui.Margin(8));}}
   if(help!=null){var slot=body.AddChildToHorizontalBox(help);if(slot!=null){slot.SetSize(new FSlateChildSize{SizeRule=ESlateSizeRule.Fill,Value=1});slot.SetPadding(Ui.Margin(8));}}
   var size=UGameplayStatics.SpawnObject(Unreal.ClassOf<USizeBox>(),tree) as USizeBox;if(size!=null){
    var viewport=UWidgetLayoutLibrary.GetViewportSize(this);float dpi=UWidgetLayoutLibrary.GetViewportScale(this);float height=dpi>0?(float)(viewport.Y/dpi)-500:550;if(height<340)height=340;
    size.SetHeightOverride(height);size.AddChild(body);column.AddChild(size);
   }else column.AddChild(body);}
  return column;
 }
 void AddTab(UHorizontalBox parent,string title){
  var tab=NativeSettingsWidgets.CategoryTab(this,title);if(tab==null)return;
  tab.SetIsSelectable(true);tab.SetIsSelected(Tabs.Count==Category,false);
  var slot=parent.AddChildToHorizontalBox(tab);if(slot!=null){slot.SetSize(new FSlateChildSize{SizeRule=ESlateSizeRule.Automatic,Value=1});slot.SetPadding(Ui.Margin(4));}Tabs.Add(tab);
 }
 public void TabClicked(UCommonButtonBase button){for(int i=0;i<Tabs.Count;i++)if(Tabs[i]==button){Category=i;Populate();return;}}
 public override void Construct(){UGameplayStatics.GetAllActorsOfClass(this,Unreal.ClassOf<ModActor>(),out List<AActor> actors);if(actors.Count>0)Manager=actors[0] as ModActor;if(Manager!=null)Manager.DiagnosticObservers++;foreach(var tab in Tabs)tab.OnButtonBaseClicked+=TabClicked;Populate();Timer.Start(this,"Refresh",0.5f,true);}
 public override void Destruct(){Timer.Stop(this,"Refresh");Rows.Clear();if(Manager!=null&&Manager.DiagnosticObservers>0)Manager.DiagnosticObservers--;foreach(var tab in Tabs)tab.OnButtonBaseClicked-=TabClicked;Manager=null;}
 public void Upscaling(){Category=0;Populate();}public void FrameGeneration(){Category=1;Populate();}public void Diagnostics(){Category=2;Populate();}
 public void Populate(){
  if(Controls==null||Manager==null)return;Controls.ClearChildren();Rows.Clear();Details=null;
  if(Category==0){Add(0);Add(1);}else if(Category==1){Add(2);Add(3);}else{Add(4);Add(5);Details=Ui.Create(this,Unreal.ClassOf<AdvancedInfoWidget>()) as AdvancedInfoWidget;if(Details!=null){var scroll=UGameplayStatics.SpawnObject(Unreal.ClassOf<UScrollBox>(),WidgetTree) as UScrollBox;if(scroll!=null){scroll.AddChild(Details);var box=UGameplayStatics.SpawnObject(Unreal.ClassOf<USizeBox>(),WidgetTree) as USizeBox;if(box!=null){box.SetHeightOverride(300);box.AddChild(scroll);Controls.AddChild(box);}}}}
  for(int i=0;i<Tabs.Count;i++)Tabs[i].SetIsSelected(i==Category,false);
  Selected=Category==0?0:Category==1?2:4;ShowHelp(Selected);Refresh();
 }
 void Add(int setting){if(Controls==null)return;var row=Ui.Create(this,Unreal.ClassOf<AdvancedSettingRow>()) as AdvancedSettingRow;if(row==null)return;row.Panel=this;row.Setting=setting;Rows.Add(row);Controls.AddChild(row);row.Refresh();}
 public void Refresh(){foreach(var row in Rows)row.Refresh();ShowHelp(Selected);}
 public void FocusRow(int setting,int direction){for(int i=0;i<Rows.Count;i++)if(Rows[i].Setting==setting){int target=i+direction;if(target<0)target=Rows.Count-1;if(target>=Rows.Count)target=0;Rows[target].SetUserFocus(World.PlayerController(this));ShowHelp(Rows[target].Setting);return;}}
 public string Title(int setting){if(setting==0)return "DLSS Model Preset";if(setting==1)return "FSR Upscaling Version";if(setting==2)return "DLSS FG Preset";if(setting==3)return "FSR FG Version";if(setting==4)return "Show Runtime Details";return "Show In-Game Diagnostics";}
 public bool OfficialSr(){return Manager!=null&&Manager.Diagnostics!=null&&Manager.Diagnostics.Fresh&&Manager.Diagnostics.State!=null&&(Manager.Diagnostics.State.FsrHardware&1)!=0;}
 public bool OfficialFg(){return Manager!=null&&Manager.Diagnostics!=null&&Manager.Diagnostics.Fresh&&Manager.Diagnostics.State!=null&&(Manager.Diagnostics.State.FsrHardware&2)!=0;}
 public List<FText> Choices(int setting){
  if(setting==0)return new List<FText>{"Auto","E - CNN (Legacy)","F - CNN (Legacy)","J - Transformer","K - Transformer","L - Transformer 2nd Gen","M - Transformer 2nd Gen"};
  if(setting==1)return new List<FText>{"Auto","3.1 - Analytical",OfficialSr()?"4.1 - Official AMD":"4.1 - INT8 (Experimental)"};
  if(setting==2)return new List<FText>{"Auto","A - Standard","B - UI Recomposition","C - Performance"};
  if(setting==3)return new List<FText>{"Auto","3.1 - Analytical",OfficialFg()?"4.0 - ML":"4.0 - ML (Unsupported GPU)"};return new List<FText>{"Off","On"};
 }
 public int Value(int setting){if(Manager==null)return 0;if(setting==0)return Manager.Advanced==null?0:AdvancedMenuClient.PresetChoice(Manager.Advanced.Hint);if(setting==1)return Manager.AdvancedFSR==null?0:Manager.AdvancedFSR.Hint;if(setting==2)return Manager.AdvancedFG==null?0:Manager.AdvancedFG.Hint;if(setting==3)return Manager.AdvancedFSRFG==null?0:Manager.AdvancedFSRFG.Hint;if(setting==4)return Manager.ShowMenuDiagnostics?1:0;return Manager.ShowDiagnostics?1:0;}
 public void Change(int setting,int direction){
  if(Manager==null)return;int count=Choices(setting).Count;int value=Value(setting)+direction;if(value<0)value=count-1;if(value>=count)value=0;
  string key=setting==0?"dlss_sr_preset":setting==1?"fsr_sr_version":setting==2?"dlss_fg_preset":setting==3?"fsr_fg_version":setting==4?"show_runtime_details":"show_runtime_info";
  if(setting<4){ModSettings.SetInt(Manager,key,value);Manager.OnSettingChanged(key,""+value);}else{ModSettings.SetBool(Manager,key,value==1);Manager.OnSettingChanged(key,value==1?"true":"false");}
  ShowHelp(setting);Refresh();Sounds.Click(this);
 }
 public void ShowHelp(int setting){
  Selected=setting;if(NativeHelp==null)return;
  if(NativeHelp.VisualData==null&&Manager!=null)foreach(var panel in Manager.Panels)if(panel!=null&&panel.ListView_Settings!=null&&panel.ListView_Settings.VisualData!=null){NativeHelp.VisualData=panel.ListView_Settings.VisualData;break;}if(NativeHelp.Text_SettingName!=null)NativeHelp.Text_SettingName.SetText(Title(setting).ToUpper());
  string body="";string footer="Recommended: AUTO";
  if(setting==0)body="Auto follows the DLSS quality mode.\n\nE / F: legacy CNN\nJ / K: Transformer\nL / M: second-generation Transformer\n\nAn explicit preset overrides Auto. Restart after changes. The driver may supersede your choice.";
  if(setting==1)body=OfficialSr()?"Choose the FSR upscaling implementation.\n\n• 3.1\nAnalytical upscaling.\n\n• 4.1\nOfficial AMD machine-learning upscaling for this GPU. Auto selects this path. Requires a compatible AMD driver.\n\nRestart after changes.":"Choose the FSR upscaling implementation.\n\n• 3.1\nAnalytical upscaling.\n\n• 4.1 INT8\nExperimental community build for this GPU. Auto uses 3.1.\n\nRestart after changes.";
  if(setting==2)body="Choose the DLSS frame-generation model.\n\nA: standard path.\nB: separate scene and UI interpolation.\nC: lighter optical-flow path.\n\nRestart after changes. C uses a driver override; see Diagnostics for whether the request was accepted.";
  if(setting==3)body=OfficialFg()?"Choose FSR frame generation independently of upscaling.\n\n• 3.1\nAnalytical frame generation.\n\n• 4.0 ML\nOfficial AMD machine-learning frame generation. Requires a compatible driver.\n\nRestart after changes.":"Choose FSR frame generation independently of upscaling.\n\n• 3.1\nAnalytical frame generation.\n\n• 4.0 ML\nRequires a supported Radeon GPU. Portable FSR upscaling does not add this support.\n\nRestart after changes.";
  if(setting==4){body="Show runtime library versions, requested presets and the last verified FSR implementations in this Diagnostics section.\n\nA loaded library or saved setting does not prove a model is active.";footer="Recommended: OFF";}
  if(setting==5){body="Show runtime activity while playing.\n\nReports advancing upscaler, generated-frame and low-latency counters. Stale or suspended activity is shown as waiting.\n\nReflex and AMD Anti-Lag 2 remain independently selectable in Video when supported.";footer="Recommended: OFF";}
  if(NativeHelp.RichText_Description!=null)NativeHelp.RichText_Description.SetText(body);
  if(NativeHelp.RichText_DynamicDetails!=null)NativeHelp.RichText_DynamicDetails.SetText("");if(NativeHelp.RichText_WarningDetails!=null)NativeHelp.RichText_WarningDetails.SetText("");if(NativeHelp.RichText_DisabledDetails!=null)NativeHelp.RichText_DisabledDetails.SetText("");
  if(Manager!=null)Manager.ShowRecommendation(NativeHelp,footer=="Recommended: OFF"?"Off":"Auto");
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
