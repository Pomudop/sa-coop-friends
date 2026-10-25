# Test stand of SA:COOP Friends (docs/fork/BACKLOG.md, INF-04): 2-4 copies of the game on one PC that join each other.
#
#   powershell -ExecutionPolicy Bypass -File tools/fork/stand.ps1 -Game "D:\Games\GTA San Andreas" [-Count 4]
#       [-Root D:\Games\gta-stand] [-Save "...\GTASAsf1.b"] [-Package dist\SA-COOP] [-NoLaunch] [-Width 960 -Height 540]
#
# -Game     a clean GTA San Andreas 1.0 US (gta_sa.exe of 14 383 616 bytes); it is only read.
# -Root     where the copies P1..P4 go, on the same disk as the game (big files are hard links: almost no space).
# -Save     a save the host (P1) loads by itself; without it the host picks one in the menu.
# -Package  the mod to install: the unpacked SA-COOP.zip of a release or of CI, or dist\SA-COOP after a local build.
# Run it again after a new build: the copies stay, the mod and the settings are put in afresh.
param(
    [Parameter(Mandatory = $true)][string]$Game,
    [ValidateRange(2, 4)][int]$Count = 4,
    [string]$Root = "",
    [string]$Save = "",
    [string]$Package = "",
    [switch]$NoLaunch,
    [int]$Width = 960,
    [int]$Height = 540
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

$Game = (Resolve-Path $Game).Path.TrimEnd('\')
$exe = Join-Path $Game "gta_sa.exe"
if (-not (Test-Path $exe)) { throw "No gta_sa.exe in $Game" }
if ((Get-Item $exe).Length -ne 14383616) {
    Write-Warning "gta_sa.exe is not 1.0 US (14 383 616 bytes): the mod will refuse to run. Downgrade the game first."
}
if (-not $Root) { $Root = Join-Path (Split-Path -Parent $Game) "gta-stand" }
New-Item -ItemType Directory -Force $Root | Out-Null
$Root = (Resolve-Path $Root).Path.TrimEnd('\')
if ($Root.StartsWith($Game + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "-Root must not be inside the game folder" }
if ([IO.Path]::GetPathRoot($Root) -ne [IO.Path]::GetPathRoot($Game)) {
    throw "-Root must be on the same disk as the game: hard links do not cross disks"
}
if (-not $Package) { $Package = Join-Path (Join-Path $repo "dist") "SA-COOP" }
if (-not (Test-Path (Join-Path $Package "coop.asi"))) {
    throw "No coop.asi in $Package. Build (python tools/build.py dist) or unpack SA-COOP.zip and pass -Package <folder>"
}
$Package = (Resolve-Path $Package).Path

# 1. the copies: files over 10 MB are hard links to the game's (a mod that edits gta3.img in place edits them all)
$files = Get-ChildItem $Game -Recurse -File
foreach ($n in 1..$Count) {
    $dst = Join-Path $Root "P$n"
    $made = 0
    foreach ($f in $files) {
        $t = Join-Path $dst $f.FullName.Substring($Game.Length + 1)
        if (Test-Path $t) { continue }  # made by an earlier run
        New-Item -ItemType Directory -Force (Split-Path $t) | Out-Null
        if ($f.Length -gt 10MB) { New-Item -ItemType HardLink -Path $t -Target $f.FullName | Out-Null }
        else { Copy-Item $f.FullName $t }
        $made++
    }
    Write-Host "P${n}: $dst ($made new files)"
}

# 2. the mod in each copy, the ASI loader as install.bat puts it (install.bat waits for a key)
foreach ($n in 1..$Count) {
    $dst = Join-Path $Root "P$n"
    robocopy $Package $dst /E /NFL /NDL /NJH /NJS /NP | Out-Null  # merges into coop\ (Copy-Item made coop\coop)
    if ($LASTEXITCODE -ge 8) { throw "Could not copy the mod into $dst (robocopy $LASTEXITCODE)" }
    if (-not (Test-Path (Join-Path $dst "vorbisHooked.dll")) -and (Test-Path (Join-Path $dst "vorbisFile.dll"))) {
        Rename-Item (Join-Path $dst "vorbisFile.dll") "vorbisHooked.dll"
        Copy-Item (Join-Path (Join-Path (Join-Path $dst "coop") "loader") "vorbisFile.dll") (Join-Path $dst "vorbisFile.dll")
    }
}

# 3. each copy's settings: P1 hosts and loads the save, the others join it; windows in a 2x2 grid
foreach ($n in 1..$Count) {
    $dst = Join-Path $Root "P$n"
    $user = Join-Path $dst "user"
    New-Item -ItemType Directory -Force $user | Out-Null
    $host1 = $n -eq 1
    if ($host1 -and $Save) { Copy-Item $Save (Join-Path $user "GTASAsf1.b") -Force }
    $x = (($n - 1) % 2) * $Width
    $y = [math]::Floor(($n - 1) / 2) * ($Height + 32)
    $connect = if ($host1) { "mode = host`r`nload_slot = $(if ($Save) { 1 } else { 0 })" } else { "mode = join`r`nhost = 127.0.0.1" }
    $ini = @"
; written by tools/fork/stand.ps1: run it again to put these back
[player]
name = P$n

[connect]
auto_connect = 1
$connect
port = 7788
max_players = $Count

[game]
windowed = 1
width = $Width
height = $Height
window_x = $x
window_y = $y
window_title = P$n
multi_instance = 1
run_in_background = 1
no_pause_menu = 1
skip_intro = 1
user_dir = $user

[debug]
overlay = 1
verbose_log = 1
firewall_prompt = 0
"@
    [IO.File]::WriteAllText((Join-Path (Join-Path $dst "coop") "coop.ini"), $ini.Replace("`r`n", "`n").Replace("`n", "`r`n"), [Text.Encoding]::ASCII)
}
Write-Host "Logs: <copy>\coop\coop.log, the host's server: P1\coop\server-data\server.log"

# 4. the host first: its server must be up before the others join
if ($NoLaunch) { return }
foreach ($n in 1..$Count) {
    $dst = Join-Path $Root "P$n"
    Start-Process (Join-Path $dst "gta_sa.exe") -WorkingDirectory $dst
    Write-Host "P$n started"
    Start-Sleep -Seconds $(if ($n -eq 1) { 15 } else { 4 })
}
