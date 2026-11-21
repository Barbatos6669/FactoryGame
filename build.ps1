$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$sources = @(
    (Join-Path $root 'src\main.cpp')
    (Join-Path $root 'src\Grid.cpp')
    (Join-Path $root 'src\Debug.cpp')
)
$projectInclude = Join-Path $root 'include'
$compilerBin = 'C:\mingw64\bin'
$compiler = Join-Path $compilerBin 'g++.exe'
$sfml = Join-Path $root 'vendor\SFML-3.1.0'
$build = Join-Path $root 'build'
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) {
    throw "Compiler missing: $compiler. Install the matching WinLibs UCRT GCC 14.2.0 64-bit package listed in README.md."
}
if (-not (Test-Path -LiteralPath (Join-Path $sfml 'include\SFML\Graphics.hpp') -PathType Leaf)) {
    throw "SFML missing: $sfml. Extract the SFML 3.1.0 GCC 14.2.0 64-bit package into vendor as described in README.md."
}
New-Item -ItemType Directory -Force -Path $build | Out-Null
& $compiler '-std=c++17' '-Wall' '-Wextra' '-g' @sources ('-I' + $projectInclude) ('-I' + (Join-Path $sfml 'include')) ('-L' + (Join-Path $sfml 'lib')) '-lsfml-graphics' '-lsfml-window' '-lsfml-system' '-o' (Join-Path $build 'FactoryGame.exe')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
'sfml-graphics-3.dll', 'sfml-window-3.dll', 'sfml-system-3.dll' | ForEach-Object {
    Copy-Item -LiteralPath (Join-Path $sfml ('bin\' + $_)) -Destination (Join-Path $build $_) -Force
}
'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll' | ForEach-Object {
    Copy-Item -LiteralPath (Join-Path $compilerBin $_) -Destination (Join-Path $build $_) -Force
}
$assets = Join-Path $root 'assets'
if (Test-Path -LiteralPath $assets -PathType Container) {
    Copy-Item -LiteralPath $assets -Destination $build -Recurse -Force
}
Write-Host 'Build successful (SFML 3.1.0). Run: .\build\FactoryGame.exe'
