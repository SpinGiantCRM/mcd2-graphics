using System.Diagnostics;
using System.Reflection;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Media;
using Avalonia.Platform.Storage;
using Avalonia.Themes.Fluent;
using MCD2.Installer;

internal static class Program {
    [STAThread] public static void Main(string[] args)=>AppBuilder.Configure<InstallerApp>().UsePlatformDetect()
        // Use GTK directly on Linux; a broken desktop portal must not strand dependency selection.
        .With(new X11PlatformOptions { UseDBusFilePicker=false }).StartWithClassicDesktopLifetime(args);
}
public sealed class InstallerApp:Application {
    public override void Initialize()=>Styles.Add(new FluentTheme());
    public override void OnFrameworkInitializationCompleted() {
        if(ApplicationLifetime is Avalonia.Controls.ApplicationLifetimes.IClassicDesktopStyleApplicationLifetime desktop)desktop.MainWindow=new InstallerWindow();
        base.OnFrameworkInitializationCompleted();
    }
}
public sealed class InstallerWindow:Window {
    readonly InstallerEngine engine;
    readonly StackPanel content=new(){Spacing=14};
    readonly TextBlock message=new(){TextWrapping=TextWrapping.Wrap};
    readonly StackPanel navigation=new(){Orientation=Orientation.Horizontal,Spacing=12};
    int page;bool busy;
    public InstallerWindow() {
        Title="Minecraft Dungeons II Graphics";Width=820;Height=690;MinWidth=700;MinHeight=550;
        engine=new(ReadResource("dependencies.lock.json"),ReadResource("manifest.json"),Path.Combine(AppContext.BaseDirectory,"payload"));
        var root=new Grid{RowDefinitions=new RowDefinitions("Auto,*,Auto"),Margin=new Thickness(28)};
        var header=new StackPanel{Spacing=8};header.Children.Add(new TextBlock{Text="MCD2 Graphics",FontSize=28,FontWeight=FontWeight.SemiBold});
        header.Children.Add(new TextBlock{Text="Install • Repair / Verify • Uninstall",Opacity=.7});
        for(int n=0;n<4;n++){int selected=n;navigation.Children.Add(Button(new[]{"1  Game","2  Requirements","3  Install","4  Support"}[n],()=>{page=selected;Draw();}));}
        header.Children.Add(navigation);root.Children.Add(header);
        var scroll=new ScrollViewer{Content=content,Margin=new Thickness(0,24,0,16)};Grid.SetRow(scroll,1);root.Children.Add(scroll);
        var footer=new StackPanel{Spacing=8};footer.Children.Add(message);footer.Children.Add(new TextBlock{Text="Saved games, account data and other mods are never part of installation or diagnostics.",Opacity=.65,TextWrapping=TextWrapping.Wrap});Grid.SetRow(footer,2);root.Children.Add(footer);Content=root;
        var detected=InstallerEngine.DetectGames().FirstOrDefault();if(detected!=null)engine.SetGame(detected);Draw();
    }
    static string ReadResource(string name){using var stream=Assembly.GetExecutingAssembly().GetManifestResourceStream(name)??throw new InvalidDataException("Installer resource missing.");using var reader=new StreamReader(stream);return reader.ReadToEnd();}
    Button Button(string label,Action action) {var b=new Button{Content=label};b.Click+=(_,_)=>{if(!busy)action();};return b;}
    Button AsyncButton(string label,Func<Task> action) {var b=new Button{Content=label};b.Click+=async(_,_)=>await Run(action);return b;}
    async Task Run(Func<Task> action) {
        if(busy)return;busy=true;navigation.IsEnabled=false;content.IsEnabled=false;message.Text="Working…";
        try{await action();}catch(Exception e){message.Text=e is InvalidDataException or InvalidOperationException?e.Message:"The operation could not be completed. Files were retained or rolled back. Check permissions, close the game, and try again.";}
        finally{busy=false;navigation.IsEnabled=true;content.IsEnabled=true;}
    }
    void Text(string text,double size=14)=>content.Children.Add(new TextBlock{Text=text,FontSize=size,TextWrapping=TextWrapping.Wrap});
    void Draw() {
        content.Children.Clear();message.Text="";
        if(page==0) {
            Text("Locate your game",22);Text("Steam libraries are detected automatically. You can choose a different Minecraft Dungeons II folder.");
            var games=InstallerEngine.DetectGames().ToList();if(games.Count>1){var choice=new ComboBox{ItemsSource=games,SelectedItem=engine.GameRoot,HorizontalAlignment=HorizontalAlignment.Stretch};choice.SelectionChanged+=(_,_)=>{if(choice.SelectedItem is string p){engine.SetGame(p);Draw();}};content.Children.Add(choice);}
            Text(engine.GameRoot??"Game not found");
            content.Children.Add(AsyncButton("Browse…",async()=>{var folders=await StorageProvider.OpenFolderPickerAsync(new(){Title="Choose Minecraft Dungeons II",AllowMultiple=false});var p=folders.SingleOrDefault()?.TryGetLocalPath();if(p!=null){engine.SetGame(p);Draw();}}));
            if(engine.GameRoot!=null){Text("Game build: "+engine.GameBuild());var check=engine.Scan()[0];Text("Status: "+(check.State=="OK"?"Supported":"Unsupported — installation is blocked"));}
            content.Children.Add(Button("Check requirements",()=>{page=1;Draw();}));
        }else if(page==1) {
            Text("Requirements",22);Text("Dependencies come from their official publishers. They are not bundled. Choose a recognized release, or enable the override to try another official version.");
            if(engine.GameRoot==null){Text("Choose the game first.");return;}
            var dependencyChecks=engine.Scan();
            foreach(var dep in engine.Dependencies) {
                var status=dependencyChecks.Single(x=>x.Name==dep.Name);var box=new StackPanel{Spacing=8};
                box.Children.Add(new TextBlock{Text=dep.Name+" — "+status.State,FontWeight=FontWeight.SemiBold});box.Children.Add(new TextBlock{Text="Required: "+dep.Requirement+" • Tested: "+dep.Version,Opacity=.75,TextWrapping=TextWrapping.Wrap});
                if(dep.MinimumVersion!=null&&dep.CompatibilityNote!=null)box.Children.Add(new TextBlock{Text=dep.CompatibilityNote,Opacity=.75,TextWrapping=TextWrapping.Wrap});
                if(status.State is "OK" or "Ready to install")box.Children.Add(new TextBlock{Text="Selected / installed: "+status.Version,Opacity=.75});
                var buttons=new StackPanel{Orientation=Orientation.Horizontal,Spacing=8};buttons.Children.Add(Button("Open official download",()=>OpenLink(dep.Url)));
                if(dep.Id=="ReShade"&&dep.Files.Keys.Any(x=>x.EndsWith("/dxgi.dll",StringComparison.Ordinal))) {
                    box.Children.Add(new TextBlock{Text="Install the full addon-support build for:\n"+Path.Combine(engine.GameRoot,InstallerEngine.Shipping)+"\nChoose DirectX 10/11/12. Complete the official installer, then Check again.",TextWrapping=TextWrapping.Wrap});
                    buttons.Children.Add(AsyncButton("Run downloaded installer…",async()=>{var selected=await SelectFile("Choose the official full-addon ReShade installer");if(selected==null)return;var lockJson=System.Text.Json.JsonDocument.Parse(ReadResource("dependencies.lock.json"));var expected=lockJson.RootElement.GetProperty("dependencies").GetProperty("ReShade").GetProperty("installerSHA256").GetString();if(InstallerEngine.DigestFile(selected)!=expected)throw new InvalidDataException("This is not the required official full-addon ReShade installer. Get it from the official link.");InstallerEngine.EnsureGameClosed();RunReShade(selected);message.Text="Finish the official ReShade installer, then choose Check again. Any license agreement remains yours to accept.";}));
                }else buttons.Children.Add(AsyncButton(dep.Files.Count>1?"Select downloaded archive…":"Select downloaded file…",async()=>{var f=await SelectFile("Select "+dep.Name+" download");if(f!=null){await Task.Run(()=>engine.SelectDependency(dep.Id,f));Draw();message.Text=engine.OverrideEnabled(dep.Id)?"Untested download selected. Compatibility is not verified.":"Download verified. It will be placed correctly when you install.";}}));
                var overrideBox=new CheckBox{Content="Try an untested dependency version",IsChecked=engine.OverrideEnabled(dep.Id)};
                overrideBox.IsCheckedChanged+=(_,_)=>{try{engine.SetDependencyOverride(dep.Id,overrideBox.IsChecked==true);Draw();}catch(Exception e){message.Text=e.Message;}};
                box.Children.Add(overrideBox);
                if(engine.OverrideEnabled(dep.Id)) {
                    box.Children.Add(new TextBlock{Text="Compatibility is not verified. Required files, hardware checks and mod integrity checks still apply. ReShade must support the mod's patched API.",TextWrapping=TextWrapping.Wrap});
                    buttons.Children.Add(AsyncButton("Use installed version",async()=>{await Task.Run(()=>engine.UseInstalledDependency(dep.Id));Draw();message.Text="Untested dependency hashes recorded for this installation.";}));
                }
                box.Children.Add(buttons);content.Children.Add(new Border{Child=box,Padding=new Thickness(12),BorderThickness=new Thickness(1),BorderBrush=Brushes.Gray,CornerRadius=new CornerRadius(6)});
            }
            var actions=new StackPanel{Orientation=Orientation.Horizontal,Spacing=12};actions.Children.Add(AsyncButton("Check again",async()=>{await Task.Run(()=>engine.Scan());Draw();message.Text=engine.Ready?"All requirements are ready.":"Resolve the missing or unsupported requirements above.";}));actions.Children.Add(Button("Continue",()=>{page=2;Draw();}));content.Children.Add(actions);
        }else if(page==2) {
            Text("Install or maintain",22);Text("Version: "+engine.Version);
            Text("Core graphics addon\nNative Video settings\nDLSS Super Resolution and DLAA\nFrame Generation preview (DLSS/DLAA + native HDR)\nHDR support\nReflex appears only when the rendering GPU and integration support it.");
            Text(engine.Ready?"Requirements ready":"Some requirements still need attention. Go back to Requirements.");
            content.Children.Add(AsyncButton("Install",async()=>{await Task.Run(()=>engine.Install());Draw();message.Text="Installed and verified. Launch normally through Steam. Open Settings → Video.";}));
            content.Children.Add(AsyncButton("Repair / Verify",async()=>{var checks=await Task.Run(()=>engine.Scan());if(checks.All(x=>x.State=="OK")){message.Text="Installation and dependencies verified.";return;}await Task.Run(()=>engine.Install(true));Draw();message.Text="Repair complete. Modified or unowned files were not overwritten.";}));
            content.Children.Add(AsyncButton("Uninstall…",async()=>{if(!await ConfirmRemoval())return;await Task.Run(()=>engine.Uninstall());Draw();message.Text="Mod and private DLSS files removed. Shared dependencies and saved games retained; startup values restored when unchanged.";}));
            content.Children.Add(Button("Launch through Steam",()=>OpenLink("steam://rungameid/1912410")));
            if(engine.GameRoot!=null)content.Children.Add(Button("Open game folder",()=>Process.Start(new ProcessStartInfo(engine.GameRoot){UseShellExecute=true})));
        }else {
            Text("Support",22);Text("Create a small diagnostic report with the mod version, game build, platform, GPU/driver and file verification results. It excludes account data, private paths, saves, raw logs and unrelated ReShade settings.");
            content.Children.Add(AsyncButton("Create diagnostic report…",async()=>{var report=await Task.Run(()=>engine.DiagnosticReport());var path=await StorageProvider.SaveFilePickerAsync(new(){Title="Save sanitized diagnostic report",SuggestedFileName="mcd2-graphics-diagnostics.txt",DefaultExtension="txt"});if(path!=null){await using var s=await path.OpenWriteAsync();using var writer=new StreamWriter(s);await writer.WriteAsync(report);message.Text="Sanitized report saved.";}}));
            content.Children.Add(Button("Third-party notices",()=>{var w=new Window{Title="Third-party notices",Width=760,Height=580};w.Content=new ScrollViewer{Content=new TextBlock{Text=ReadResource("notices.txt"),TextWrapping=TextWrapping.Wrap,Margin=new Thickness(20)}};w.Show(this);}));
            content.Children.Add(Button("Installation help",()=>OpenLink("https://github.com/SpinGiantCRM/mcd2-graphics/blob/main/INSTALL.md")));
            content.Children.Add(Button("Report a problem",()=>OpenLink("https://github.com/SpinGiantCRM/mcd2-graphics/issues/new/choose")));
            Text("Online compatibility is unverified. Offline play is safer; follow the publisher’s terms. Advanced RenoDX settings remain in ReShade.");
        }
    }
    async Task<string?> SelectFile(string title) {var selected=await StorageProvider.OpenFilePickerAsync(new(){Title=title,AllowMultiple=false});return selected.SingleOrDefault()?.TryGetLocalPath();}
    static void OpenLink(string url)=>Process.Start(new ProcessStartInfo(url){UseShellExecute=true});
    void RunReShade(string path) {
        if(OperatingSystem.IsWindows()){Process.Start(new ProcessStartInfo(path){UseShellExecute=true});return;}
        var steamapps=Directory.GetParent(engine.GameRoot!)?.Parent?.FullName??throw new InvalidDataException("Steam library unavailable.");
        var compat=Path.Combine(steamapps,"compatdata/1912410");var config=Path.Combine(compat,"config_info");
        if(!File.Exists(config))throw new InvalidDataException("Launch this game through Steam once to create its Proton prefix, then retry.");
        var lines=File.ReadAllLines(config);var tool=lines.Skip(1).FirstOrDefault(x=>x.Contains("/files/",StringComparison.Ordinal));
        if(tool==null||lines.Length<4)throw new InvalidDataException("The game's current Proton path could not be detected. Use the official ReShade installer with your existing game prefix, then Check again.");
        var proton=Path.Combine(tool[..tool.IndexOf("/files/",StringComparison.Ordinal)],"proton");if(!File.Exists(proton))throw new InvalidDataException("The selected Proton tool is unavailable.");
        var p=new ProcessStartInfo(proton){UseShellExecute=false};p.ArgumentList.Add("run");p.ArgumentList.Add(path);
        p.Environment["STEAM_COMPAT_DATA_PATH"]=compat;p.Environment["STEAM_COMPAT_CLIENT_INSTALL_PATH"]=lines[3];
        p.Environment["STEAM_COMPAT_INSTALL_PATH"]=engine.GameRoot;
        p.Environment["STEAM_COMPAT_LIBRARY_PATHS"]=Directory.GetParent(steamapps)!.FullName;
        Process.Start(p);
    }
    async Task<bool> ConfirmRemoval() {
        var dialog=new Window{Title="Uninstall MCD2 Graphics?",Width=450,Height=220,CanResize=false,WindowStartupLocation=WindowStartupLocation.CenterOwner};
        var panel=new StackPanel{Margin=new Thickness(24),Spacing=16};panel.Children.Add(new TextBlock{Text="Remove this mod's verified files and its private DLSS copy? Shared dependencies, saves and other preferences remain. Startup values are restored only if unchanged.",TextWrapping=TextWrapping.Wrap});
        var buttons=new StackPanel{Orientation=Orientation.Horizontal,Spacing=12};var yes=new Button{Content="Uninstall"};yes.Click+=(_,_)=>dialog.Close(true);var no=new Button{Content="Cancel"};no.Click+=(_,_)=>dialog.Close(false);buttons.Children.Add(no);buttons.Children.Add(yes);panel.Children.Add(buttons);dialog.Content=panel;return await dialog.ShowDialog<bool>(this);
    }
}
