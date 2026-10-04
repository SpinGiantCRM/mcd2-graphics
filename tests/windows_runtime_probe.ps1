param(
    [int]$Seconds = 60,
    [int]$WaitForLaunchSeconds = 120,
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
$exited = $false
for ($sample = 0; $sample -le $Seconds; $sample++) {
    $current = Get-Process -Id $gameProcessId -ErrorAction SilentlyContinue
    if (-not $current) { $exited = $true; break }
    $modules = @($current.Modules | ForEach-Object { $_.ModuleName })
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
    renoDXLoaded = $modules -contains 'renodx-ue-extended.addon64'
    samples = $samples
}
$result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $Output
if ($exited) { throw 'Game exited during observation; inspect crash and ReShade logs.' }
if (-not $result.mcd2AddonLoaded) { throw 'Game stayed alive, but MCD2 Graphics was not loaded.' }
Write-Output 'MCD2 Graphics loaded and the game stayed alive throughout the observation. Verify menu/gameplay and fallback separately.'
