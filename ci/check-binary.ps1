# Checks the Windows plugin against the single-binary rules (plan B8):
#   - it depends only on DLLs that ship with Windows (no VC++ runtime,
#     no OpenSSL or other third-party DLL);
#   - VcmpPluginInit is its only export.
#
# usage: ci/check-binary.ps1 build/windows-release/bin/LuaPlugin_x64.dll
param([Parameter(Mandatory = $true)][string] $Dll)

$ErrorActionPreference = 'Stop'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dumpbin = Get-ChildItem (Join-Path $vs 'VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe') |
    Sort-Object FullName | Select-Object -Last 1
if (-not $dumpbin) { throw 'dumpbin.exe not found' }

$failed = $false
function Fail([string] $message) {
    Write-Host "check-binary: $message"
    $script:failed = $true
}

# Dependencies: the lines between the header and the summary.
$dependents = & $dumpbin.FullName /nologo /dependents $Dll
$dependencies = @()
$inList = $false
foreach ($line in $dependents) {
    if ($line -match 'Image has the following dependencies') { $inList = $true; continue }
    if ($inList -and $line -match '^\s*Summary') { break }
    if ($inList -and $line.Trim() -ne '') { $dependencies += $line.Trim() }
}
$system32 = Join-Path $env:SystemRoot 'System32'
$redistributable = '^(vcruntime|msvcp|concrt|vccorlib|ucrtbase|api-ms-win-crt-)'
foreach ($dependency in $dependencies) {
    if ($dependency -match $redistributable) {
        Fail "depends on $dependency (the C/C++ runtime must be linked statically)"
    } elseif (-not (Test-Path (Join-Path $system32 $dependency))) {
        Fail "depends on $dependency, which is not a Windows system DLL"
    }
}

# Exports: rows of "ordinal hint RVA name".
$exportLines = & $dumpbin.FullName /nologo /exports $Dll
$exports = @()
foreach ($line in $exportLines) {
    if ($line -match '^\s+\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]{8}\s+(\S+)') { $exports += $Matches[1] }
}
if (($exports -join ' ') -ne 'VcmpPluginInit') {
    Fail "exports must be exactly VcmpPluginInit, got: $($exports -join ' ')"
}

Write-Host "check-binary: $Dll"
Write-Host "  dependencies: $($dependencies -join ' ')"
Write-Host "  exports:      $($exports -join ' ')"
if ($failed) {
    Write-Host 'check-binary: FAILED'
    exit 1
}
Write-Host 'check-binary: OK'
