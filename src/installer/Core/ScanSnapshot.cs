namespace MCD2.Installer;

// A UI view, never transaction authority. A mutation always calls RefreshScan.
public sealed record ScanSnapshot(string? GameRoot, string PackageVersion, IReadOnlyList<Check> Checks) {
 public bool Ready => GameRoot!=null && Checks.Where(c=>c.Name!="Graphics mod payload").All(c=>c.State is "OK" or "Ready to install" or "Untested override");
}
