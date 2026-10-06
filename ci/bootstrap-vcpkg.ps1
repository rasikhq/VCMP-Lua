# Puts vcpkg at the baseline pinned in vcpkg-configuration.json into the
# directory given as the first argument (cloning it the first time) and
# builds the tool. Windows counterpart of ci/bootstrap-vcpkg.sh.
param([Parameter(Mandatory = $true)][string] $Root)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true

$config = Get-Content (Join-Path $PSScriptRoot '..\vcpkg-configuration.json') -Raw | ConvertFrom-Json
$baseline = $config.'default-registry'.baseline

# A full clone: vcpkg reads older port versions (the Lua 5.4.8 override) from
# git history, and a partial clone would fetch them from GitHub mid-build.
if (-not (Test-Path (Join-Path $Root '.git'))) {
    git clone https://github.com/microsoft/vcpkg $Root
}
$head = git -C $Root rev-parse HEAD
if ($head -ne $baseline -or -not (Test-Path (Join-Path $Root 'vcpkg.exe'))) {
    git -C $Root fetch origin
    git -C $Root checkout --quiet --detach $baseline
    & (Join-Path $Root 'bootstrap-vcpkg.bat') -disableMetrics
}
