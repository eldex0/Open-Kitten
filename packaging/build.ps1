$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root

function Resolve-BuildTool([string]$name, [string[]]$fallbacks) {
    $command = Get-Command $name -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    foreach ($candidate in $fallbacks) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) { return $candidate }
    }
    throw "Required build tool not found: $name"
}

function Invoke-Native([string]$executable, [string[]]$arguments) {
    & $executable @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $executable $($arguments -join ' ')"
    }
}

$cargoExe = Resolve-BuildTool 'cargo' @(
    (Join-Path $env:USERPROFILE '.cargo\bin\cargo.exe')
)
$cmakeExe = Resolve-BuildTool 'cmake' @(
    (Join-Path $env:ProgramFiles 'CMake\bin\cmake.exe')
)

Push-Location native\webui
try {
    if (Get-Command npm -ErrorAction SilentlyContinue) {
        $npmExe = (Get-Command npm).Source
        Invoke-Native $npmExe @('install', '--no-audit', '--no-fund')
        Invoke-Native $npmExe @('run', 'check')
        Invoke-Native $npmExe @('run', 'build')
    } elseif (Get-Command pnpm -ErrorAction SilentlyContinue) {
        $pnpmExe = (Get-Command pnpm).Source
        Invoke-Native $pnpmExe @('install', '--frozen-lockfile')
        Invoke-Native $pnpmExe @('run', 'check')
        Invoke-Native $pnpmExe @('run', 'build')
    } else {
        throw 'Open Kitten requires npm or pnpm to build the TypeScript WebUI.'
    }
} finally {
    Pop-Location
}

Invoke-Native $cargoExe @('test', '--manifest-path',
    'native\rust\security_core\Cargo.toml', '--locked')
$configure = @('-S', '.', '-B', 'build\native')
if (-not (Test-Path -LiteralPath 'build\native\CMakeCache.txt')) {
    $vsWhere = Join-Path ${env:ProgramFiles(x86)} `
        'Microsoft Visual Studio\Installer\vswhere.exe'
    $knownMsBuild = Join-Path ${env:ProgramFiles(x86)} `
        'Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
    $hasVisualStudio = (Test-Path -LiteralPath $knownMsBuild)
    if (Test-Path -LiteralPath $vsWhere) {
        $installation = & $vsWhere -latest -products '*' `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        $hasVisualStudio = $hasVisualStudio -or [bool]$installation
    }
    if ($hasVisualStudio) {
        $configure += @('-G', 'Visual Studio 17 2022', '-A', 'x64')
    } elseif ((Get-Command ninja -ErrorAction SilentlyContinue) -and
              (Get-Command cl -ErrorAction SilentlyContinue)) {
        $configure += @('-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release')
    } else {
        throw 'A Visual Studio 2022 C++ toolchain or an initialized Ninja/MSVC environment is required.'
    }
    if (-not $env:CMAKE_PREFIX_PATH) {
        $qtConfig = Get-ChildItem -LiteralPath 'C:\Qt' -Filter 'Qt6Config.cmake' -File -Recurse `
            -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
        if ($qtConfig) {
            $qtPrefix = $qtConfig.Directory.Parent.Parent.Parent.FullName
            $configure += "-DCMAKE_PREFIX_PATH=$qtPrefix"
        }
    }
}
$configure += "-DCARGO_EXECUTABLE=$cargoExe"
Invoke-Native $cmakeExe $configure
Invoke-Native $cmakeExe @('--build', 'build\native', '--config', 'Release')
$ctestExe = Join-Path (Split-Path -Parent $cmakeExe) 'ctest.exe'
Invoke-Native $ctestExe @('--test-dir', 'build\native', '--build-config',
    'Release', '--output-on-failure')

$output = Join-Path $root 'dist\OpenKitten'
Invoke-Native $cmakeExe @('--install', 'build\native', '--config', 'Release',
    '--prefix', $output)

$selfTest = Start-Process -FilePath (Join-Path $output 'OpenKitten.exe') `
    -ArgumentList '--self-test' -WindowStyle Hidden -Wait -PassThru
if ($selfTest.ExitCode -ne 0) {
    throw "Packaged Open Kitten self-test failed with exit code $($selfTest.ExitCode)."
}
$uiSmokeTest = Start-Process -FilePath (Join-Path $output 'OpenKitten.exe') `
    -ArgumentList '--ui-smoke-test' -WindowStyle Hidden -Wait -PassThru
if ($uiSmokeTest.ExitCode -ne 0) {
    throw "Packaged Open Kitten UI smoke test failed with exit code $($uiSmokeTest.ExitCode)."
}
$privateSmokeTest = Start-Process -FilePath (Join-Path $output 'OpenKitten.exe') `
    -ArgumentList '--private-smoke-test' -WindowStyle Hidden -Wait -PassThru
if ($privateSmokeTest.ExitCode -ne 0) {
    throw "Packaged Open Kitten private-mode smoke test failed with exit code $($privateSmokeTest.ExitCode)."
}

$manifest = Join-Path $root 'dist\SHA256SUMS.txt'
$lines = Get-ChildItem -LiteralPath $output -File -Recurse |
    Sort-Object FullName |
    ForEach-Object {
        $relative = $_.FullName.Substring($output.Length).TrimStart([char[]]@('\', '/')).Replace('\', '/')
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
        "$hash  $relative"
    }
[IO.File]::WriteAllLines($manifest, $lines, [Text.UTF8Encoding]::new($false))

$releaseDirectory = Join-Path $root 'release'
New-Item -ItemType Directory -Path $releaseDirectory -Force | Out-Null
$portableArchive = Join-Path $releaseDirectory 'OpenKitten-2.0.0-portable.zip'
Compress-Archive -Path (Join-Path $output '*') -DestinationPath $portableArchive `
    -CompressionLevel Optimal -Force

[string[]]$isccCandidates = @(
    (Get-Command iscc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source),
    (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
    (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe')
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }
if (-not $isccCandidates) {
    throw 'Inno Setup 6 is required to create the Open Kitten 2.0 installer.'
}
$isccExe = $isccCandidates[0]
Invoke-Native $isccExe @('packaging\installer.iss')

$releaseManifest = Join-Path $releaseDirectory 'SHA256SUMS.txt'
$releaseLines = Get-ChildItem -LiteralPath $releaseDirectory -File |
    Where-Object { $_.Name -ne 'SHA256SUMS.txt' } |
    Sort-Object Name |
    ForEach-Object {
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
        "$hash  $($_.Name)"
    }
[IO.File]::WriteAllLines($releaseManifest, $releaseLines, [Text.UTF8Encoding]::new($false))
Write-Host "Native build complete: $output"
