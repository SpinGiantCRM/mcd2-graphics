// MIT licensed reusable helpers for Blueprint Loader custom settings widgets.
// References the game's existing widgets; no game assets are redistributed.
using NeoRune;
using UE.CoreUObject;
using UE.UMG;
using UE.GameSettings;
using UE.Angelscript;
public static class NativeSettingsWidgets {
 public static UGameSettingDetailView? HelpCard(UObject owner){return Ui.GameWidget(owner,"/SpicewoodSettings/Spicewood/UI/SettingsScreen/W_SettingsDetailView.W_SettingsDetailView_C") as UGameSettingDetailView;}
 public static UAS_TabButton_Settings? CategoryTab(UObject owner,string title){
  var tab=Ui.GameWidget(owner,"/SpicewoodSettings/Spicewood/UI/SettingsScreen/W_TabButton_Settings.W_TabButton_Settings_C") as UAS_TabButton_Settings;if(tab==null)return null;
  tab.SetButtonText(title);tab.PreConstruct(false);tab.UpdateButtonText(title);
  tab.SetShouldDeselectUponUnhover(false);tab.SetShouldDeselectUponLosingFocus(false);if(tab.TextIconSwitch!=null&&tab.TextOvr!=null)tab.TextIconSwitch.SetActiveWidget(tab.TextOvr);
  SetText(tab.TextOvr,title);return tab;
 }
 public static void SetText(UWidget? widget,string text){
  if(widget==null)return;var plain=widget as UTextBlock;if(plain!=null)plain.SetText(text);var rich=widget as URichTextBlock;if(rich!=null)rich.SetText(text);
  var panel=widget as UPanelWidget;if(panel!=null)for(int i=0;i<panel.GetChildrenCount();i++)SetText(panel.GetChildAt(i),text);
 }
}
