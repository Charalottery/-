# run_tests.ps1
# For each testfile.txt under test/2025代码生成公共测试程序库:
#  - copy it into Compiler.exe directory as testfile.txt
#  - run Compiler.exe (working dir = src)
#  - collect llvm_ir.txt and append testfile + llvm_ir to a combined output file

# Optional parameter: comma-separated group names (e.g. 'A,B' or 'D')
param(
    [string]$Groups = ""
)

$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $scriptDir) { $scriptDir = (Get-Location).Path }
$projectRoot = Resolve-Path (Join-Path $scriptDir "..")
$testRoot = Join-Path $projectRoot "test\2025代码生成公共测试程序库"
$outputRoot = Join-Path $projectRoot "test_outputs_2025_codegen"
# Clear previous outputs to avoid stale results
if (Test-Path $outputRoot) {
    # remove all files and dirs under outputRoot but keep the folder
    Remove-Item -Path (Join-Path $outputRoot '*') -Recurse -Force -ErrorAction SilentlyContinue
} else {
    New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
}
$srcDir = Join-Path $projectRoot "src"
$compilerExe = Join-Path $srcDir "Compiler.exe"
if (-not (Test-Path $compilerExe)) {
    Write-Error "Compiler.exe not found at $compilerExe"
    exit 1
}

$testfiles = Get-ChildItem -Path $testRoot -Filter 'testfile.txt' -Recurse -File -ErrorAction SilentlyContinue
if (-not $testfiles) {
    Write-Host "No testfile.txt found under $testRoot"
    exit 0
}

# We'll write separate files per testcase instead of one combined file

$resolvedTestRoot = (Resolve-Path $testRoot).Path

# If Groups parameter provided, filter testfiles to only those groups
if ($Groups -and $Groups.Trim() -ne "") {
    $allowed = $Groups -split '[,;]' | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' }
    if ($allowed.Count -eq 0) {
        Write-Host "No valid groups specified in -Groups. Exiting."
        exit 0
    }
    Write-Host ("Filtering to groups: {0}" -f ($allowed -join ','))
    $testfiles = $testfiles | Where-Object {
        $relLocal = $_.FullName.Substring($resolvedTestRoot.Length).TrimStart('\','/')
        $first = ($relLocal -split '[\\/]')[0]
        $allowed -contains $first
    }
    if (-not $testfiles) {
        Write-Host ("No testfile.txt found for groups: {0}" -f ($allowed -join ','))
        exit 0
    }
}

foreach ($tf in $testfiles) {
    $rel = $tf.FullName.Substring($resolvedTestRoot.Length).TrimStart('\','/')
    $header = $rel -replace 'testfile.txt$','' -replace '[\\/]',' ' -replace '\s+',' ' -replace '_$',''
    $header = $header.Trim()
    if (-not $header) { $header = [System.Guid]::NewGuid().ToString() }

    Write-Host "Processing: $rel -> Header: $header"

    # Read original testfile content
    $testContent = Get-Content -Path $tf.FullName -Raw -ErrorAction SilentlyContinue
    if ($testContent -eq $null) { $testContent = '' }

    $destTestfile = Join-Path $srcDir "testfile.txt"
    $backup = $null
    if (Test-Path $destTestfile) {
        $backup = "$destTestfile.bak"
        Copy-Item $destTestfile $backup -Force
    }
    Copy-Item $tf.FullName $destTestfile -Force

    # Run compiler in src directory
    Push-Location $srcDir
    try {
        & "$compilerExe" > $null 2> $null
    } catch {
        Write-Warning ("Compiler failed for {0}: {1}" -f $rel, $_)
    }
    Pop-Location

    # Read mips output from src (new program output)
    $mipsSrc = Join-Path $srcDir "mips.txt"
    $mipsContent = ''
    if (Test-Path $mipsSrc) {
        $mipsContent = Get-Content -Path $mipsSrc -Raw -ErrorAction SilentlyContinue
        if ($mipsContent -eq $null) { $mipsContent = '' }
    }

    # Create a filesystem-safe case name for output files
    $caseFileSafe = ($header -replace '\s+','_') -replace '[\\/:]','_' -replace '[^\w\-]',''
    if (-not $caseFileSafe) { $caseFileSafe = [System.Guid]::NewGuid().ToString() }

    # Write individual files: <case>_testfile.txt and <case>_mips.txt
    $testOutPath = Join-Path $outputRoot ("{0}_testfile.txt" -f $caseFileSafe)
    $mipsOutPath = Join-Path $outputRoot ("{0}_mips.txt" -f $caseFileSafe)

    # Save original testfile content
    if ($testContent -ne '') {
        $testContent | Out-File -FilePath $testOutPath -Encoding UTF8 -Force
    } else {
        New-Item -Path $testOutPath -ItemType File -Force | Out-Null
    }

    # Save mips output
    if ($mipsContent -ne '') {
        $mipsContent | Out-File -FilePath $mipsOutPath -Encoding UTF8 -Force
    } else {
        New-Item -Path $mipsOutPath -ItemType File -Force | Out-Null
    }

    # Also write stdout/stderr for debugging convenience
    $stdoutPath = Join-Path $outputRoot ("{0}_stdout.txt" -f $caseFileSafe)
    $stderrPath = Join-Path $outputRoot ("{0}_stderr.txt" -f $caseFileSafe)
    # If compiler produced files in src from previous runs, try to capture them
    if (Test-Path (Join-Path $srcDir "stdout.txt")) {
        Copy-Item (Join-Path $srcDir "stdout.txt") $stdoutPath -Force -ErrorAction SilentlyContinue
    } else {
        # create empty files to keep consistent output
        New-Item -Path $stdoutPath -ItemType File -Force | Out-Null
    }
    if (Test-Path (Join-Path $srcDir "stderr.txt")) {
        Copy-Item (Join-Path $srcDir "stderr.txt") $stderrPath -Force -ErrorAction SilentlyContinue
    } else {
        New-Item -Path $stderrPath -ItemType File -Force | Out-Null
    }

    # restore original testfile in src or remove the copied one
    if ($backup) {
        Copy-Item $backup $destTestfile -Force
        Remove-Item $backup -Force -ErrorAction SilentlyContinue
    } else {
        Remove-Item $destTestfile -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ("Done. Outputs saved to: {0}" -f $outputRoot)
