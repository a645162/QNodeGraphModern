[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [string]$QtPrefix = '',
    [string]$BuildDirectory = '',
    [string]$StagingDirectory = ''
)

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $repoRoot "build\$Configuration"
}
if ([string]::IsNullOrWhiteSpace($StagingDirectory)) {
    $StagingDirectory = Join-Path $repoRoot "build\package\$Configuration"
}
if (-not (Test-Path $BuildDirectory)) {
    throw "Build directory was not found: $BuildDirectory"
}

cmake --install $BuildDirectory --config $Configuration --prefix $StagingDirectory
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ([string]::IsNullOrWhiteSpace($QtPrefix) -and $env:QNODEGRAPH_QT_PREFIX) {
    $QtPrefix = $env:QNODEGRAPH_QT_PREFIX
}
if ([string]::IsNullOrWhiteSpace($QtPrefix)) {
    $defaultQtPrefix = 'C:\Qt\6.12.0\llvm-mingw_64'
    if (Test-Path (Join-Path $defaultQtPrefix 'bin\windeployqt.exe')) {
        $QtPrefix = $defaultQtPrefix
    }
}

$deployTool = if ([string]::IsNullOrWhiteSpace($QtPrefix)) {
    'windeployqt.exe'
} else {
    Join-Path $QtPrefix 'bin\windeployqt.exe'
}
if (-not (Get-Command $deployTool -ErrorAction SilentlyContinue) -and
    -not (Test-Path $deployTool)) {
    throw "windeployqt.exe was not found. Pass -QtPrefix or add it to PATH."
}

$examples = @(
    (Join-Path $BuildDirectory 'examples\QNodeGraph.BasicDemo\QNodeGraph.BasicDemo.exe'),
    (Join-Path $BuildDirectory 'examples\QNodeGraph.ImagePipelineDemo\QNodeGraph.ImagePipelineDemo.exe')
)
foreach ($example in $examples) {
    if (-not (Test-Path $example)) {
        throw "Example executable was not found: $example"
    }
    $target = Join-Path $StagingDirectory (Split-Path $example -Leaf)
    Copy-Item $example $target -Force
    & $deployTool --no-translations --qmldir (Join-Path $repoRoot 'examples') $target
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
