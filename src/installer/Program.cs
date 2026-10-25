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
    readonly string[] detectedGames;
    readonly StackPanel content=new(){Spacing=14};
    readonly TextBlock message=new(){TextWrapping=TextWrapping.Wrap};
    readonly StackPanel navigation=new(){Orientation=Orientation.Horizontal,Spacing=12};
    int page;bool busy,advanced,watchDownloads;
    CancellationTokenSource? downloadCancellation;
    readonly Button cancelDownload=new(){Content="Cancel download",IsVisible=false};
    readonly DependencyAcquisition acquisition=new(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"MCD2Graphics","DownloadCache"));
    public InstallerWindow() {
        Title="Minecraft Dungeons II Graphics";Width=820;Height=690;MinWidth=700;MinHeight=550;
        engine=new(ReadResource("dependencies.lock.json"),ReadResource("manifest.json"),Path.Combine(AppContext.BaseDirectory,"payload"));
        engine.UseContract(ReadResource("mcd2-install.json"),ReadResource("manifest.json"),ReadResource("dependencies.lock.json"));
        var root=new Grid{RowDefinitions=new RowDefinitions("Auto,*,Auto"),Margin=new Thickness(28)};
        var header=new StackPanel{Spacing=8};header.Children.Add(new TextBlock{Text="MCD2 Graphics",FontSize=28,FontWeight=FontWeight.SemiBold});
        header.Children.Add(new TextBlock{Text="Install • Repair / Verify • Uninstall",Opacity=.7});
        for(int n=0;n<3;n++){int selected=n;navigation.Children.Add(Button(new[]{"Game","Install / Maintain","Support"}[n],()=>{page=selected;Draw();}));}
        header.Children.Add(navigation);root.Children.Add(header);
        var scroll=new ScrollViewer{Content=content,Margin=new Thickness(0,24,0,16)};Grid.SetRow(scroll,1);root.Children.Add(scroll);
        var footer=new StackPanel{Spacing=8};cancelDownload.Click+=(_,_)=>downloadCancellation?.Cancel();footer.Children.Add(cancelDownload);footer.Children.Add(message);footer.Children.Add(new TextBlock{Text="Saved games, account data and other mods are never part of installation or diagnostics.",Opacity=.65,TextWrapping=TextWrapping.Wrap});Grid.SetRow(footer,2);root.Children.Add(footer);Content=root;
        var locations=detectedGames=InstallerEngine.DetectGames().Distinct().ToArray();
        var detected=locations.Length==1?locations[0]:null;
        if(detected!=null)try{engine.SetGame(detected);}catch(Exception e)when(e is InvalidDataException or IOException or UnauthorizedAccessException){engine.ClearGame();}
        if(engine.GameRoot!=null)page=1;
        Activated+=async(_,_)=>{if(watchDownloads&&!busy)await Run(async()=>{watchDownloads=false;await ImportRecognizedDownloads();Draw();});};
        Draw();
    }
    static string ReadResource(string name){using var stream=Assembly.GetExecutingAssembly().GetManifestResourceStream(name)??throw new InvalidDataException("Installer resource missing.");using var reader=new StreamReader(stream);return reader.ReadToEnd();}
    Button Button(string label,Action action) {var b=new Button{Content=label};b.Click+=(_,_)=>{if(!busy)action();};return b;}
    Button AsyncButton(string label,Func<Task> action) {var b=new Button{Content=label};b.Click+=async(_,_)=>await Run(action);return b;}
    async Task Run(Func<Task> action) {
        if(busy)return;busy=true;navigation.IsEnabled=false;content.IsEnabled=false;message.Text="Working…";
        try{await action();}catch(OperationCanceledException){message.Text="Download cancelled. Nothing was installed.";}catch(Exception e){message.Text=e is InvalidDataException or InvalidOperationException?e.Message:"The operation could not be completed. Files were retained or rolled back. Check permissions, close the game, and try again.";}
        finally{busy=false;navigation.IsEnabled=true;content.IsEnabled=true;}
    }
    void Text(string text,double size=14)=>content.Children.Add(new TextBlock{Text=text,FontSize=size,TextWrapping=TextWrapping.Wrap});
    void Draw() {
        content.Children.Clear();message.Text="";
        if(page==0) {
            Text("Locate your game",22);Text("Choose the installation folder or shipping executable. Steam libraries are detected automatically. Supported builds are checked before installation. Game Pass on Linux uses your existing Xodus-managed prefix.");
            var games=detectedGames;if(games.Length>1){var choice=new ComboBox{ItemsSource=games,SelectedItem=engine.GameRoot,HorizontalAlignment=HorizontalAlignment.Stretch};choice.SelectionChanged+=async(_,_)=>{if(choice.SelectedItem is string p)await Run(()=>ChooseGame(p));};content.Children.Add(choice);}
            Text(engine.GameRoot??"Game not found");
            content.Children.Add(AsyncButton("Browse folder…",async()=>{
                var selected=await StorageProvider.OpenFolderPickerAsync(new(){Title="Choose Minecraft Dungeons II",AllowMultiple=false});
                if(selected.SingleOrDefault()?.TryGetLocalPath() is string path)await ChooseGame(path);
            }));
            content.Children.Add(AsyncButton("Choose game executable…",async()=>{
                var selected=await StorageProvider.OpenFilePickerAsync(new(){Title="Choose the game's shipping executable",AllowMultiple=false,
                    FileTypeFilter=new[]{new FilePickerFileType("Windows executable"){Patterns=new[]{"*.exe"}}}});
                if(selected.SingleOrDefault()?.TryGetLocalPath() is string path)await ChooseGame(path);
            }));
            if(engine.GameRoot!=null){Text("Game build: "+engine.GameBuild());var check=engine.Snapshot.Checks[0];Text("Status: "+(check.State=="OK"?"Supported":"Unsupported — installation is blocked"));}
            content.Children.Add(Button("Check requirements",()=>{page=1;Draw();}));
        }else if(page==1) {
            if(engine.GameRoot==null){Text("Choose the game first.",22);content.Children.Add(Button("Choose game",()=>{page=0;Draw();}));return;}
            var transition=engine.Transition;
            if(transition.Owner is InstallOwner.Vortex or InstallOwner.Conflict){Text("Change installation manager",22);Text(transition.NextAction);content.Children.Add(AsyncButton("Check again",async()=>{await Task.Run(()=>engine.RefreshScan());Draw();}));return;}
            var snapshot=engine.Snapshot;var installed=snapshot.Checks.Single(x=>x.Name=="Graphics mod payload").State;
            Text(installed=="OK"?"Installed":snapshot.Ready?"Ready to install":"Requirements need attention",22);
            Text("MCD2 Graphics "+engine.Version+" • "+engine.Profile!.Edition);
            Text(engine.Description);
            foreach(var check in snapshot.Checks.Where(c=>c.Name!="Graphics mod payload"&&c.State is not ("OK" or "Ready to install")))Text(check.Name+": "+check.State);
            var actions=new StackPanel{Orientation=Orientation.Horizontal,Spacing=10};
            if(!snapshot.Ready)actions.Children.Add(AsyncButton("Get missing requirements",GetMissingRequirements));
            actions.Children.Add(AsyncButton("Import downloads…",ImportFiles));
            actions.Children.Add(AsyncButton("Check again",async()=>{await Task.Run(()=>engine.RefreshScan());Draw();}));content.Children.Add(actions);
            if(snapshot.Ready){
                content.Children.Add(AsyncButton(installed=="Not installed"?"Install":"Update / Repair",async()=>{await Task.Run(()=>engine.Install(installed!="Not installed"));Draw();message.Text="Installed and verified. Launch normally from your game launcher. Open Settings → Video.";}));
                if(installed!="Not installed")content.Children.Add(AsyncButton("Uninstall…",async()=>{if(!await ConfirmRemoval())return;await Task.Run(()=>engine.Uninstall());Draw();message.Text="Removed owned files. Shared requirements, preferences and saves retained.";}));
            }else if(installed!="Not installed")content.Children.Add(AsyncButton("Uninstall…",async()=>{if(await ConfirmRemoval()){await Task.Run(()=>engine.Uninstall());Draw();}}));
            if(transition.Owner==InstallOwner.Standalone){
                Text(transition.NextAction);
                content.Children.Add(AsyncButton("Prepare for Vortex…",async()=>{if(!await ConfirmRemoval(true))return;await Task.Run(()=>engine.Uninstall());Draw();message.Text="Ready for Vortex. Install and enable its matching MCD2 Graphics package for this game. Your preferences, saves and shared requirements are retained.";}));
            }
            Text("Start the game from "+(engine.Profile.Launcher==GameLauncher.Steam?"Steam":engine.Profile.Launcher==GameLauncher.Xodus?"Xodus, using its existing prefix":"the Xbox app")+" after installation.");
            var details=new CheckBox{Content="Advanced requirements and untested versions",IsChecked=advanced};details.IsCheckedChanged+=(_,_)=>{advanced=details.IsChecked==true;Draw();};content.Children.Add(details);
            if(advanced)foreach(var dep in engine.Dependencies){
                var status=snapshot.Checks.Single(x=>x.Name==dep.Name);Text(dep.Name+" — "+status.State+" • Tested: "+dep.Version);
                content.Children.Add(Button("Open publisher page",()=>OpenLink(dep.Url)));
                content.Children.Add(AsyncButton("Import "+dep.Name,async()=>{var f=await SelectFile("Select "+dep.Name);if(f!=null){await Task.Run(()=>engine.SelectDependency(dep.Id,f));Draw();}}));
                var allow=new CheckBox{Content="Try an untested "+dep.Name+" version",IsChecked=engine.OverrideEnabled(dep.Id)};
                allow.IsCheckedChanged+=(_,_)=>{try{engine.SetDependencyOverride(dep.Id,allow.IsChecked==true);Draw();}catch(Exception e){message.Text=e.Message;}};content.Children.Add(allow);
                if(engine.OverrideEnabled(dep.Id))content.Children.Add(AsyncButton("Use installed version",async()=>{await Task.Run(()=>engine.UseInstalledDependency(dep.Id));Draw();}));
                if(dep.Id=="ReShade"&&dep.Files.Keys.Any(x=>x.EndsWith("/dxgi.dll",StringComparison.Ordinal)))content.Children.Add(AsyncButton("Run official full-addon installer…",async()=>{var f=await SelectFile("Choose official ReShade installer");if(f==null)return;using var pins=System.Text.Json.JsonDocument.Parse(ReadResource("dependencies.lock.json"));if(InstallerEngine.DigestFile(f)!=pins.RootElement.GetProperty("dependencies").GetProperty("ReShade").GetProperty("installerSHA256").GetString())throw new InvalidDataException("Choose the pinned official full-addon installer.");InstallerEngine.EnsureGameClosed();RunReShade(f);}));
            }
        }else {
            Text("Support",22);Text("Create a small diagnostic report with the mod version, game build, platform, GPU/driver and file verification results. It excludes account data, private paths, saves, raw logs and unrelated ReShade settings.");
            content.Children.Add(AsyncButton("Create diagnostic report…",async()=>{var report=await Task.Run(()=>engine.DiagnosticReport());var path=await StorageProvider.SaveFilePickerAsync(new(){Title="Save sanitized diagnostic report",SuggestedFileName="mcd2-graphics-diagnostics.txt",DefaultExtension="txt"});if(path!=null){await using var s=await path.OpenWriteAsync();using var writer=new StreamWriter(s);await writer.WriteAsync(report);message.Text="Sanitized report saved.";}}));
            content.Children.Add(Button("Third-party notices",()=>{var w=new Window{Title="Third-party notices",Width=760,Height=580};w.Content=new ScrollViewer{Content=new TextBlock{Text=ReadResource("notices.txt"),TextWrapping=TextWrapping.Wrap,Margin=new Thickness(20)}};w.Show(this);}));
            content.Children.Add(Button("Installation help",()=>OpenLink("https://github.com/SpinGiantCRM/mcd2-graphics/blob/main/INSTALL.md")));
            content.Children.Add(Button("Report a problem",()=>OpenLink("https://github.com/SpinGiantCRM/mcd2-graphics/issues/new/choose")));
            Text("Online compatibility is unverified. Offline play is safer; follow the publisher’s terms. Advanced RenoDX settings remain in ReShade.");
        }
    }
    RequirementPlan Requirements()=>RequirementPlanner.Resolve(engine.Contract!,new(engine.Profile!.Edition,engine.Profile.Environment),engine.Dependencies,engine.Snapshot.Checks);
    async Task ImportRecognizedDownloads(){
        if(engine.GameRoot==null)return;
        var missing=Requirements().Requirements.Where(r=>r.InstalledRelease==null).Select(r=>r.Dependency).ToArray();
        var home=Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        var files=await Task.Run(()=>acquisition.FindDownloads(Path.Combine(home,"Downloads"),missing));
        foreach(var file in files)await ImportMatching(file,false);
    }
    async Task ImportMatching(string file,bool report){
        var hash=await Task.Run(()=>InstallerEngine.DigestFile(file));
        var deps=engine.Dependencies.Where(d=>DependencyAcquisition.AcceptedHashes(d).Contains(hash)).ToArray();
        if(deps.Length==0){if(report)throw new InvalidDataException("That download does not match a supported requirement. Use Advanced for an explicit untested-version override.");return;}
        foreach(var dep in deps)await Task.Run(()=>engine.SelectDependency(dep.Id,file));
    }
    async Task ImportFiles(){
        var selected=await StorageProvider.OpenFilePickerAsync(new(){Title="Import requirement downloads",AllowMultiple=true});
        foreach(var f in selected)if(f.TryGetLocalPath() is {} path)await ImportMatching(path,true);
        Draw();message.Text="Downloads checked. Verified files will be placed when you install.";
    }
    async Task GetMissingRequirements(){
        await ImportRecognizedDownloads();
        foreach(var r in Requirements().Requirements){
            var state=engine.Snapshot.Checks.Single(c=>c.Name==r.Dependency.Name).State;
            if(state is "OK" or "Ready to install" or "Untested override")continue;
            if(acquisition.FindCached(r.Dependency) is {} cached){await Task.Run(()=>engine.SelectDependency(r.Dependency.Id,cached));continue;}
            if(r.Acquisition.Method=="direct"){
                using var cancellation=new CancellationTokenSource();downloadCancellation=cancellation;cancelDownload.IsVisible=true;
                try{var staged=await acquisition.StageAsync(r,cancellation.Token,new Progress<AcquisitionProgress>(p=>message.Text="Downloading "+p.Requirement+"…"));await Task.Run(()=>engine.SelectDependency(r.Dependency.Id,staged.Path));}
                finally{downloadCancellation=null;cancelDownload.IsVisible=false;}
                continue;
            }
            OpenLink(r.Acquisition.Url);watchDownloads=true;Draw();message.Text="Download "+r.Dependency.Name+" from its publisher, then return here. Login and licence prompts remain yours to complete. Downloads are recognised automatically; Import downloads also works offline.";return;
        }
        Draw();message.Text=engine.Ready?"All requirements are ready. Choose Install.":"Check the requirements above.";
    }
    async Task ChooseGame(string selection) {
        engine.ClearGame();
        try {
            var location=GameLocation.Resolve(selection);string? prefix=null;
            if(!OperatingSystem.IsWindows()&&location.Edition==GameEdition.WinGDK) {
                var selected=await StorageProvider.OpenFolderPickerAsync(new(){Title="Choose this game's existing Xodus Wine prefix (pfx)",AllowMultiple=false});
                prefix=selected.SingleOrDefault()?.TryGetLocalPath();
                if(prefix==null)return;
            }
            engine.SetGame(location.Root,location.Edition,prefix);
        } finally {Draw();}
    }
    async Task<string?> SelectFile(string title) {var selected=await StorageProvider.OpenFilePickerAsync(new(){Title=title,AllowMultiple=false});return selected.SingleOrDefault()?.TryGetLocalPath();}
    static void OpenLink(string url)=>Process.Start(new ProcessStartInfo(url){UseShellExecute=true});
    void RunReShade(string path) {
        if(OperatingSystem.IsWindows()){Process.Start(new ProcessStartInfo(path){UseShellExecute=true});return;}
        if(engine.Profile?.Launcher==GameLauncher.Xodus)throw new InvalidDataException("Use Xodus's existing runtime and prefix to run the official ReShade installer, then Check again. This installer does not change the managed runtime.");
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
    async Task<bool> ConfirmRemoval(bool forVortex=false) {
        var dialog=new Window{Title=forVortex?"Prepare for Vortex?":"Uninstall MCD2 Graphics?",Width=450,Height=220,CanResize=false,WindowStartupLocation=WindowStartupLocation.CenterOwner};
        var panel=new StackPanel{Margin=new Thickness(24),Spacing=16};panel.Children.Add(new TextBlock{Text="Remove this mod's verified files and its private DLSS copy? Shared dependencies, saves and other preferences remain. Startup values are restored only if unchanged.",TextWrapping=TextWrapping.Wrap});
        var buttons=new StackPanel{Orientation=Orientation.Horizontal,Spacing=12};var yes=new Button{Content=forVortex?"Remove owned files for Vortex":"Uninstall"};yes.Click+=(_,_)=>dialog.Close(true);var no=new Button{Content="Cancel"};no.Click+=(_,_)=>dialog.Close(false);buttons.Children.Add(no);buttons.Children.Add(yes);panel.Children.Add(buttons);dialog.Content=panel;return await dialog.ShowDialog<bool>(this);
    }
}
