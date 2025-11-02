# package_compiler.ps1
# Usage: run this script from anywhere; it will create a zip archive under the my-compiler folder
# It collects: .cpp, .c, .hpp, .h, Makefile, *.json
param(
    [string]$ProjectRoot = "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)"
)

Set-Location -Path $ProjectRoot

# Use fixed archive name and remove any existing archive named Compiler.zip before creating
$zipName = "Compiler.zip"
$zipPath = Join-Path $ProjectRoot $zipName
if (Test-Path $zipPath) {
    Write-Output "Removing existing archive: $zipPath"
    Remove-Item -Path $zipPath -Force
}

# Collect only desired src subdirectories and selected root files
$files = @()
$srcDir = Join-Path $ProjectRoot 'src'
$subdirs = @('backend','frontend','midend','error')
foreach ($d in $subdirs) {
    $p = Join-Path $srcDir $d
    if (Test-Path $p) {
        $files += Get-ChildItem -Path $p -Recurse -File -ErrorAction SilentlyContinue
    }
}

# include specific root files under src if present
$rootFiles = @('CMakeLists.txt','config.json','main.cpp','Makefile')
foreach ($rf in $rootFiles) {
    $fp = Join-Path $srcDir $rf
    if (Test-Path $fp) { $files += Get-Item -Path $fp }
}

# remove duplicates and filter out unwanted extensions (.exe, .txt)
# but keep src/CMakeLists.txt even though it has a .txt extension
$files = $files | Select-Object -Unique | Where-Object {
    if ($_.Name -eq 'CMakeLists.txt') { return $true }
    $_.Extension -notin '.exe', '.txt'
}

if ($files.Count -eq 0) {
    Write-Error "No files found to package in $ProjectRoot"
    exit 1
}

# Create the zip using System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

if (Test-Path $zipPath) { Remove-Item $zipPath }

$zip = [System.IO.Compression.ZipFile]::Open($zipPath, 'Create')
try {
    foreach ($f in $files) {
        # store files relative to the src directory (so archive contains src's contents at the top level)
        $fullPath = (Resolve-Path -Path $f.FullName).Path
        if ($fullPath.StartsWith($srcDir, [System.StringComparison]::OrdinalIgnoreCase)) {
            $entryName = $fullPath.Substring($srcDir.Length).TrimStart('\')
        } else {
            # fallback to project-root-relative if file isn't under src for any reason
            $entryName = $fullPath.Substring($ProjectRoot.Length).TrimStart('\')
        }
        # normalize to forward slashes for zip entries
        $entryName = $entryName -replace '\\', '/'
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $f.FullName, $entryName)
    }
} finally {
    $zip.Dispose()
}

Write-Output "Created archive: $zipPath"
Write-Output "Included $($files.Count) files."