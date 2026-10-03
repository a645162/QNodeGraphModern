[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [switch]$ImagePipelineDemo,
    [switch]$BasicDemo,
    [string]$QtPrefix = '',
    [string]$BuildDirectory = ''
)

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$buildScript = Join-Path $repoRoot 'scripts\build\build.ps1'
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $repoRoot "build\$Configuration"
}
& $buildScript -Configuration $Configuration -QtPrefix $QtPrefix -BuildDirectory $BuildDirectory
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ([string]::IsNullOrWhiteSpace($QtPrefix) -and $env:QNODEGRAPH_QT_PREFIX) {
    $QtPrefix = $env:QNODEGRAPH_QT_PREFIX
}
if ([string]::IsNullOrWhiteSpace($QtPrefix)) {
    $defaultQtPrefix = 'C:\Qt\6.12.0\llvm-mingw_64'
    if (Test-Path (Join-Path $defaultQtPrefix 'bin')) {
        $QtPrefix = $defaultQtPrefix
    }
}
if (-not [string]::IsNullOrWhiteSpace($QtPrefix)) {
    $env:Path = "$(Join-Path $QtPrefix 'bin');$env:Path"
}

$target = if ($ImagePipelineDemo) {
    'QNodeGraph.ImagePipelineDemo'
} else {
    'QNodeGraph.BasicDemo'
}
$executable = Get-ChildItem -Path $BuildDirectory -Recurse -File -Filter "$target.exe" |
    Select-Object -First 1
if ($null -eq $executable) {
    throw "Executable was not found below: $BuildDirectory"
}

& $executable.FullName
exit $LASTEXITCODE
