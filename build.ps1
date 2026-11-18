$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$sfml = Join-Path $root 'vendor\SFML-3.1.0'
$build = Join-Path $root 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null
& 'C:\mingw64\bin\g++.exe' '-std=c++17' '-Wall' '-Wextra' '-g' (Join-Path $root 'main.cpp') ('-I' + (Join-Path $sfml 'include')) ('-L' + (Join-Path $sfml 'lib')) '-lsfml-graphics' '-lsfml-window' '-lsfml-system' '-o' (Join-Path $build 'FactoryGame.exe')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
'sfml-graphics-3.dll', 'sfml-window-3.dll', 'sfml-system-3.dll' | ForEach-Object {
    Copy-Item -LiteralPath (Join-Path $sfml ('bin\' + $_)) -Destination (Join-Path $build $_) -Force
}
Write-Host 'Build successful (SFML 3.1.0). Run: .\build\FactoryGame.exe'
