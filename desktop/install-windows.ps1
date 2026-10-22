# Baut den Lyrics Translator direkt in die Spotify-Desktop-App ein (ohne Spicetify).
# Aufruf: Doppelklick auf install-windows.bat  /  Entfernen: uninstall-windows.bat
param([switch]$Uninstall)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$marker = "<!-- lyrics-translator -->"
$scriptName = "lyrics-translator.js"
$source = Join-Path $PSScriptRoot $scriptName
$utf8 = New-Object System.Text.UTF8Encoding($false)

function Fail($msg) {
    Write-Host ""
    Write-Host "FEHLER: $msg" -ForegroundColor Red
    exit 1
}

# Spotify legt seine Oberflaeche entweder als Paket (Apps\xpui.spa) oder entpackt (Apps\xpui\index.html) ab.
$appsDirs = @(
    (Join-Path $env:APPDATA "Spotify\Apps"),
    (Join-Path $env:LOCALAPPDATA "Spotify\Apps")
)
$spa = $null
$folder = $null
foreach ($d in $appsDirs) {
    if (Test-Path (Join-Path $d "xpui.spa")) { $spa = Join-Path $d "xpui.spa"; break }
    if (Test-Path (Join-Path $d "xpui\index.html")) { $folder = Join-Path $d "xpui"; break }
}
if (-not $spa -and -not $folder) {
    if (Get-AppxPackage -Name "SpotifyAB.SpotifyMusic" -ErrorAction SilentlyContinue) {
        Fail "Du hast die Microsoft-Store-Version von Spotify. Die laesst sich nicht veraendern. Bitte Spotify im Store deinstallieren und von https://www.spotify.com/download installieren, dann diesen Installer nochmal starten."
    }
    Fail "Spotify wurde nicht gefunden (gesucht in $($appsDirs -join ' und ')). Bitte Spotify von https://www.spotify.com/download installieren."
}
if (-not (Test-Path $source)) { Fail "$scriptName fehlt neben diesem Installer. Bitte den ganzen Ordner 'desktop' entpacken." }

$target = if ($spa) { $spa } else { $folder }
Write-Host "Spotify gefunden: $target"
Get-Process -Name "Spotify" -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 1200

function Add-Script([string]$html) {
    if ($html.Contains($marker)) { return $html }
    # Direkt nach <head>, damit das Script vor Spotifys eigenem Code geladen wird.
    $regex = [regex]"<head[^>]*>"
    if (-not $regex.IsMatch($html)) { Fail "Spotifys index.html sieht anders aus als erwartet. Bitte melden." }
    return $regex.Replace($html, { param($m) $m.Value + $marker + "<script src=""$scriptName""></script>" }, 1)
}

function Read-Entry($zip, $name) {
    $entry = $zip.GetEntry($name)
    if (-not $entry) { return $null }
    $reader = New-Object System.IO.StreamReader($entry.Open(), $utf8)
    try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
}

function Write-Entry($zip, $name, $text) {
    $old = $zip.GetEntry($name)
    if ($old) { $old.Delete() }
    $writer = New-Object System.IO.StreamWriter($zip.CreateEntry($name).Open(), $utf8)
    try { $writer.Write($text) } finally { $writer.Dispose() }
}

try {
    if ($spa) {
        $backup = "$spa.bak"
        if ($Uninstall) {
            if (Test-Path $backup) { Move-Item $backup $spa -Force; Write-Host "Lyrics Translator wurde entfernt." }
            else { Write-Host "Keine Sicherung gefunden - der Mod ist vermutlich nicht installiert." }
            exit 0
        }
        $zip = [System.IO.Compression.ZipFile]::OpenRead($spa)
        try { $patched = (Read-Entry $zip "index.html").Contains($marker) } finally { $zip.Dispose() }
        # Sicherung nur vom Original. Nach einem Spotify-Update ist xpui.spa wieder original -> neue Sicherung.
        if (-not $patched) { Copy-Item $spa $backup -Force }

        $zip = [System.IO.Compression.ZipFile]::Open($spa, [System.IO.Compression.ZipArchiveMode]::Update)
        try {
            Write-Entry $zip $scriptName ([System.IO.File]::ReadAllText($source, $utf8))
            Write-Entry $zip "index.html" (Add-Script (Read-Entry $zip "index.html"))
        } finally { $zip.Dispose() }
    } else {
        $index = Join-Path $folder "index.html"
        $backup = "$index.bak"
        if ($Uninstall) {
            if (Test-Path $backup) { Move-Item $backup $index -Force }
            Remove-Item (Join-Path $folder $scriptName) -ErrorAction SilentlyContinue
            Write-Host "Lyrics Translator wurde entfernt."
            exit 0
        }
        $html = [System.IO.File]::ReadAllText($index, $utf8)
        if (-not $html.Contains($marker)) { Copy-Item $index $backup -Force }
        Copy-Item $source (Join-Path $folder $scriptName) -Force
        [System.IO.File]::WriteAllText($index, (Add-Script $html), $utf8)
    }
} catch {
    Fail "Konnte Spotify nicht veraendern: $($_.Exception.Message)`nIst Spotify wirklich geschlossen? Notfalls den PC neu starten und den Installer direkt danach ausfuehren."
}

Write-Host ""
Write-Host "Fertig! Lyrics Translator ist in Spotify eingebaut." -ForegroundColor Green
Write-Host "Unten rechts neben dem Mikrofon-Knopf (Lyrics) ist jetzt ein Uebersetzen-Knopf."
Write-Host "Oeffne in Spotify die Lyrics - die Uebersetzung steht unter jeder Zeile."
Write-Host "Nach einem Spotify-Update einfach diesen Installer nochmal starten."

$exe = Join-Path (Split-Path (Split-Path $target)) "Spotify.exe"
if (-not (Test-Path $exe)) { $exe = Join-Path (Split-Path (Split-Path (Split-Path $target))) "Spotify.exe" }
if (Test-Path $exe) { Start-Process $exe }
