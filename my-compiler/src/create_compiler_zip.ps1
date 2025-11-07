# create_compiler_zip.ps1
# Usage: run this script from anywhere; it will operate in the script's directory (the repo's src folder).

$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location -Path $scriptDir

$zipName = "Compiler.zip"
# We will place the submission zip at the repository level (parent of src)
$destZip = Resolve-Path -LiteralPath (Join-Path $scriptDir "..\$zipName") -ErrorAction SilentlyContinue
if ($destZip) { $destZip = $destZip.Path } else { $destZip = (Join-Path $scriptDir "..\$zipName") }

# Remove previous zip if exists at the parent (submission) location
if (Test-Path $destZip) {
    Write-Host "Removing existing $destZip..."
    Remove-Item -Path $destZip -Force
}

# Candidate items to include (accept both 'back' and 'backend' to be tolerant)
$items = @(
    'back',
    'backend',
    'error',
    'frontend',
    'midend',
    'optimize',
    'utils',
    'CMakeLists.txt',
    'config.json',
    'main.cpp',
    'Makefile'
)

# Keep only existing items
$existing = $items | Where-Object { Test-Path $_ }

if ($existing.Count -eq 0) {
    Write-Host "No specified files or directories found in $scriptDir. Nothing to add to $zipName." -ForegroundColor Yellow
    exit 1
}

Write-Host "Creating $zipName with the following items:`n  " -NoNewline
$existing | ForEach-Object { Write-Host "$_" }

# Compress-Archive will overwrite if -Force is used
Compress-Archive -Path $existing -DestinationPath $destZip -Force

if (Test-Path $destZip) {
    Write-Host "Successfully created $zipName at: $destZip" -ForegroundColor Green
} else {
    Write-Host "Failed to create $zipName" -ForegroundColor Red
    exit 2
}
