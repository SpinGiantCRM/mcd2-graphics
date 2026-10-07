param(
    [int]$Seconds = 60,
    [int]$WaitForLaunchSeconds = 120,
    # Update 2 has a second independently loaded addon. Keep legacy SR-only probes usable.
    [switch]$RequireDisplayLatency,
    # FG replaces the DXGI route and adds independently loaded modules.
    [switch]$RequireFG,
    [Parameter(Mandatory = $true)][string]$Output
)
$ErrorActionPreference = 'Stop'
if (-not $IsWindows) { throw 'This probe requires Windows PowerShell 7.' }
if ($Seconds -lt 11 -or $WaitForLaunchSeconds -lt 1) { throw 'Observe at least 11 seconds.' }
$deadline = (Get-Date).AddSeconds($WaitForLaunchSeconds)
do {
    $candidates = @(Get-Process -Name Dungeons-Win64-Shipping -ErrorAction SilentlyContinue)
    if ($candidates.Count -eq 1) { break }
    if ($candidates.Count -gt 1) { throw 'Close duplicate game processes before testing.' }
    Start-Sleep -Seconds 1
} while ((Get-Date) -lt $deadline)
if ($candidates.Count -ne 1) { throw 'Game did not launch before the deadline.' }
$gameProcessId = $candidates[0].Id
$started = $candidates[0].StartTime
$observed = Get-Date
$samples = @()
$modules = @()
$fgModuleHashes = @{}
$exited = $false
for ($sample = 0; $sample -le $Seconds; $sample++) {
    $current = Get-Process -Id $gameProcessId -ErrorAction SilentlyContinue
    if (-not $current) { $exited = $true; break }
    $modules = @($current.Modules | ForEach-Object { $_.ModuleName })
    if ($RequireFG -and ($sample -eq 0 -or $sample -eq $Seconds)) {
        $gameDirectory = Split-Path -Parent $current.MainModule.FileName
        $fgModuleHashes = @{}
        foreach ($module in $current.Modules) {
            if ((Split-Path -Parent $module.FileName) -eq $gameDirectory -and $module.ModuleName -in @('dxgi.dll', 'd3d12.asi', 'fg-sdk-bridge.dll', 'mcd2-fg-guide-recon.addon64', 'mcd2-display-latency.addon64', 'mcd2-graphics.addon64')) {
                $fgModuleHashes[$module.ModuleName] = (Get-FileHash -LiteralPath $module.FileName -Algorithm SHA256).Hash.ToLowerInvariant()
            }
        }
    }
    $samples += [pscustomobject]@{
        secondsFromObservation = [math]::Round(((Get-Date)-$observed).TotalSeconds, 1)
        secondsFromLaunch = [math]::Round(((Get-Date)-$started).TotalSeconds, 1)
        responding = $current.Responding
        cpuSeconds = $current.CPU
    }
    if ($sample -lt $Seconds) { Start-Sleep -Seconds 1 }
}
$result = [ordered]@{
    observedAt = $observed.ToString('o')
    processId = $gameProcessId
    launchTime = $started.ToString('o')
    observationSeconds = [math]::Round(((Get-Date)-$observed).TotalSeconds, 1)
    exitedDuringObservation = $exited
    mcd2AddonLoaded = $modules -contains 'mcd2-graphics.addon64'
    displayLatencyAddonLoaded = $modules -contains 'mcd2-display-latency.addon64'
    fgBootstrapLoaded = $fgModuleHashes.ContainsKey('dxgi.dll')
    fgFrameworkLoaded = $fgModuleHashes.ContainsKey('d3d12.asi')
    fgBridgeLoaded = $fgModuleHashes.ContainsKey('fg-sdk-bridge.dll')
    fgGuideAddonLoaded = $fgModuleHashes.ContainsKey('mcd2-fg-guide-recon.addon64')
    fgModuleSHA256 = $fgModuleHashes
    renoDXLoaded = $modules -contains 'renodx-ue-extended.addon64'
    samples = $samples
}
$result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $Output
if ($exited) { throw 'Game exited during observation; inspect crash and ReShade logs.' }
if (-not $result.mcd2AddonLoaded) { throw 'Game stayed alive, but MCD2 Graphics was not loaded.' }
if ($RequireDisplayLatency -and -not $result.displayLatencyAddonLoaded) { throw 'Game stayed alive, but the Update 2 display/latency addon was not loaded.' }
if ($RequireFG -and $fgModuleHashes.Count -ne 6) { throw 'Game stayed alive, but the complete FG trial module chain was not loaded from the game directory.' }
Write-Output 'MCD2 Graphics loaded and the game stayed alive throughout the observation. Verify menu/gameplay and fallback separately.'
