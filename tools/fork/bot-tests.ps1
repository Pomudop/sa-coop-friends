# Server tests of SA:COOP Friends (docs/fork/BACKLOG.md, INF-05): coop-bot.exe starts a fresh coop-server.exe for each
# scenario and plays it with fake players. No game is needed.
#   powershell -ExecutionPolicy Bypass -File tools/fork/bot-tests.ps1 [-DebugBuild] [arguments of coop-bot.exe]
# Build first: python tools/build.py server bot. The exit code is the bot's: 0 passed, 1 a scenario failed, 2 usage.
# No param() block: an attribute there makes the script an advanced one, whose own -Debug and -Verbose would take the
# bot's arguments.
$debugBuild = $args -contains "-DebugBuild"
$botArgs = @($args | Where-Object { $_ -ne "-DebugBuild" })
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$bin = Join-Path $root ("build\" + $(if ($debugBuild) { "debug" } else { "release" }) + "\bin")
$bot = Join-Path $bin "coop-bot.exe"
if (-not (Test-Path $bot)) {
    Write-Host "coop-bot.exe not found in $bin. Build it: python tools/build.py server bot"
    exit 2
}
& $bot --server (Join-Path $bin "coop-server.exe") @botArgs
exit $LASTEXITCODE
