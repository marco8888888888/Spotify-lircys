# Baut den Lyrics Translator direkt in die Spotify-Desktop-App ein (ohne Spicetify).
# Aufruf: Doppelklick auf install-windows.bat  –  Entfernen: uninstall-windows.bat
param([switch]$Uninstall)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$marker = "<!-- lyrics-translator -->"
$scriptName = "lyrics-translator.js"
$source = Join-Path $PSScriptRoot $scriptName

$candidates = @(
    (Join-Path $env:APPDATA "Spotify\Apps\xpui.spa"),
    (Join-Path $env:LOCALAPPDATA "Spotify\Apps\xpui.spa")
)
$spa = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $spa) {
    if (Get-AppxPackage -Name "SpotifyAB.SpotifyMusic" -ErrorAction SilentlyContinue) {
        throw "Du hast die Microsoft-Store-Version von Spotify. Die lässt sich nicht verändern. Bitte deinstallieren und Spotify von https://www.spotify.com/download installieren."
    }
    throw "Spotify wurde nicht gefunden. Bitte Spotify von https://www.spotify.com/download installieren."
}
$backup = "$spa.bak"

Write-Host "Spotify gefunden: $spa"
Get-Process -Name "Spotify" -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 800

function Test-Patched($path) {
    $zip = [System.IO.Compression.ZipFile]::OpenRead($path)
    try {
        $entry = $zip.GetEntry("index.html")
        $reader = New-Object System.IO.StreamReader($entry.Open())
        try { return $reader.ReadToEnd().Contains($marker) } finally { $reader.Dispose() }
    } finally { $zip.Dispose() }
}

if ($Uninstall) {
    if (Test-Path $backup) {
        Copy-Item $backup $spa -Force
        Remove-Item $backup
        Write-Host "Lyrics Translator wurde entfernt."
    } else {
        Write-Host "Keine Sicherung gefunden - der Mod ist vermutlich nicht installiert."
    }
    exit 0
}

if (-not (Test-Path $source)) { throw "$scriptName fehlt neben diesem Installer." }

# Sicherung nur vom Original anlegen. Nach einem Spotify-Update ist xpui.spa wieder original -> neue Sicherung.
if (-not (Test-Patched $spa)) {
    Copy-Item $spa $backup -Force
}

$zip = [System.IO.Compression.ZipFile]::Open($spa, [System.IO.Compression.ZipArchiveMode]::Update)
try {
    $old = $zip.GetEntry($scriptName)
    if ($old) { $old.Delete() }
    $entry = $zip.CreateEntry($scriptName)
    $writer = New-Object System.IO.StreamWriter($entry.Open(), (New-Object System.Text.UTF8Encoding($false)))
    try { $writer.Write([System.IO.File]::ReadAllText($source, [System.Text.Encoding]::UTF8)) } finally { $writer.Dispose() }

    $index = $zip.GetEntry("index.html")
    $reader = New-Object System.IO.StreamReader($index.Open())
    try { $html = $reader.ReadToEnd() } finally { $reader.Dispose() }
    if (-not $html.Contains($marker)) {
        # Direkt nach <head>, damit das Script vor Spotifys eigenem Code geladen wird.
        $regex = [regex]"<head[^>]*>"
        $html = $regex.Replace($html, { param($m) $m.Value + $marker + "<script src=""$scriptName""></script>" }, 1)
        $index.Delete()
        $index = $zip.CreateEntry("index.html")
        $writer = New-Object System.IO.StreamWriter($index.Open(), (New-Object System.Text.UTF8Encoding($false)))
        try { $writer.Write($html) } finally { $writer.Dispose() }
    }
} finally { $zip.Dispose() }

Write-Host ""
Write-Host "Fertig! Lyrics Translator ist in Spotify eingebaut."
Write-Host "Unten rechts neben dem Lyrics-Knopf ist jetzt ein Uebersetzen-Knopf (oder Strg+Umschalt+L)."
Write-Host "Nach einem Spotify-Update einfach diesen Installer nochmal starten."

$exe = Join-Path (Split-Path (Split-Path $spa)) "Spotify.exe"
if (Test-Path $exe) { Start-Process $exe }
