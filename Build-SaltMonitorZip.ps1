# Build-SaltMonitorZip.ps1
#Requires -Version 5.1
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# --- 1) Dateien/Ordner definieren (ANPASSEN) ---
# Einzelne Dateien und/oder Ordner. Wildcards sind erlaubt.
$Include = @(
"SaltMonitor.ino", 
"SaltMonitor.ino.bin", 
"SaltMonitor.ino.merged.bin", 
"Waage.cpp", 
"Waage.h", 
"WifiConfigManager.cpp", 
"WifiConfigManager.h"

)

# --- 2) Version abfragen ---
$version = Read-Host "Bitte Version eingeben (x_nm, z.B. 1.2.3 oder build-2025-09-01)"
if ([string]::IsNullOrWhiteSpace($version)) {
    Write-Error "Keine Version eingegeben. Abbruch."
    exit 1
}

# Ungültige Dateinamenzeichen durch Unterstrich ersetzen
$sanitizedVersion = ($version -replace '[<>:"/\\|?*]', '_').Trim()

# --- 3) Ausgabepfad/Datei ---
$scriptDir = Split-Path -Parent $PSCommandPath
$distDir    = Join-Path $scriptDir "archiv"
if (-not (Test-Path $distDir)) { New-Item -ItemType Directory -Path $distDir | Out-Null }

$zipPath    = Join-Path $distDir ("SaltMonitor-{0}.zip" -f $sanitizedVersion)

# Falls ZIP existiert: Rückfrage
if (Test-Path $zipPath -PathType Leaf) {
    $answer = Read-Host "Die Datei '$zipPath' existiert bereits. Überschreiben? (j/n)"
    if ($answer -notmatch '^(j|y)$') {
        Write-Host "Abgebrochen."
        exit 0
    }
    Remove-Item $zipPath -Force
}

# --- 4) Dateien auflösen ---
# Achtung: Wenn du hier nur Dateien angibst, werden in der ZIP standardmäßig keine Ordnerstruktur erhalten.
# Wenn du Ordner angibst (z. B. 'assets'), bleibt die Struktur erhalten.
$items = Get-ChildItem -Path $Include -Recurse -File -ErrorAction SilentlyContinue

if (-not $items -or $items.Count -eq 0) {
    Write-Error "Keine passenden Dateien gefunden. Prüfe die Include-Liste."
    exit 1
}

# --- 5) ZIP erstellen ---
Compress-Archive -Path $items.FullName -DestinationPath $zipPath -CompressionLevel Optimal -Force

Write-Host "Fertig: $zipPath"