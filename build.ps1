param(
    [switch]$Rebuild
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$compilerBin = 'C:\mingw64\bin'
$compiler = Join-Path $compilerBin 'g++.exe'
$make = Join-Path $compilerBin 'mingw32-make.exe'
$sfml = Join-Path $root 'vendor\SFML-3.1.0'
$build = Join-Path $root 'build'

function Copy-RuntimeFile($source, $destination) {
    $sourceFile = Get-Item -LiteralPath $source
    $destinationFile = Get-Item -LiteralPath $destination -ErrorAction SilentlyContinue
    if ($destinationFile -and $destinationFile.Length -eq $sourceFile.Length -and
        $destinationFile.LastWriteTimeUtc -eq $sourceFile.LastWriteTimeUtc) {
        return
    }

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
}

if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) {
    throw "Compiler missing: $compiler. Install the matching WinLibs UCRT GCC 14.2.0 64-bit package listed in README.md."
}
if (-not (Test-Path -LiteralPath $make -PathType Leaf)) {
    throw "GNU Make missing: $make. Check the WinLibs toolchain installation in $compilerBin."
}
if (-not (Test-Path -LiteralPath (Join-Path $sfml 'include\SFML\Graphics.hpp') -PathType Leaf)) {
    throw "SFML missing: $sfml. Extract the SFML 3.1.0 GCC 14.2.0 64-bit package into vendor as described in README.md."
}

$makeArguments = @("CXX=$($compiler.Replace('\', '/'))")
if ($Rebuild) {
    $makeArguments += '--always-make'
}

New-Item -ItemType Directory -Force -Path (Join-Path $build 'obj') | Out-Null
# Only one build task can write to this output directory at a time.
$buildLock = [System.IO.File]::Open((Join-Path $build '.build.lock'), 'OpenOrCreate', 'Write', 'None')
Push-Location -LiteralPath $root
try {
    & $make @makeArguments
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    foreach ($dll in 'sfml-graphics-3.dll', 'sfml-window-3.dll', 'sfml-system-3.dll') {
        Copy-RuntimeFile (Join-Path $sfml ('bin\' + $dll)) (Join-Path $build $dll)
    }
    foreach ($dll in 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll') {
        Copy-RuntimeFile (Join-Path $compilerBin $dll) (Join-Path $build $dll)
    }

    $assets = Join-Path $root 'assets'
    if (Test-Path -LiteralPath $assets -PathType Container) {
        foreach ($file in Get-ChildItem -LiteralPath $assets -Recurse -File) {
            $relativePath = $file.FullName.Substring($assets.Length + 1)
            Copy-RuntimeFile $file.FullName (Join-Path $build ('assets\' + $relativePath))
        }
    }
    Write-Host 'Build successful (SFML 3.1.0). Run .\build\FactoryGame.exe'
}
finally {
    Pop-Location
    $buildLock.Dispose()
}
