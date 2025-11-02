# Remove existing zip if it exists
$zipPath = Join-Path -Path (Get-Location) -ChildPath 'ÎÄ·¨½â¶Á.zip'
if (Test-Path $zipPath) {
    Write-Output "Removing existing $zipPath"
    Remove-Item $zipPath -Force
}

# Collect all .txt files in current directory
$txtFiles = Get-ChildItem -Path . -Filter '*.txt' | ForEach-Object { $_.FullName }
if ($txtFiles.Count -eq 0) {
    Write-Output "No .txt files found to zip."
    exit 0
}

# Create zip
Write-Output "Creating $zipPath from .txt files..."
Compress-Archive -Path $txtFiles -DestinationPath $zipPath -Force

if (Test-Path $zipPath) {
    Write-Output "Created $zipPath successfully."
} else {
    Write-Output "Failed to create $zipPath."
}
