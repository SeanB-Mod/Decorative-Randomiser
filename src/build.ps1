param(
    [string]$Compiler
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $Compiler) {
    $command = Get-Command 'clang++.exe' -ErrorAction SilentlyContinue
    if ($command) { $Compiler = $command.Source }
}
if (-not $Compiler -or -not (Test-Path -LiteralPath $Compiler)) {
    throw 'clang++.exe was not found. Add LLVM-MinGW to PATH or run build.ps1 -Compiler C:\path\to\clang++.exe'
}
$clang = $Compiler
$build = Join-Path $root 'build'
New-Item -ItemType Directory -Force $build | Out-Null
& $clang -std=c++17 -O2 -Wall -Wextra -Werror -static (Join-Path $PSScriptRoot 'test_random_logic.cpp') -o (Join-Path $build 'test_random_logic.exe')
if ($LASTEXITCODE -ne 0) { throw 'Unit test compilation failed' }
& (Join-Path $build 'test_random_logic.exe')
if ($LASTEXITCODE -ne 0) { throw 'Unit tests failed' }
& $clang -std=c++17 -O2 -Wall -Wextra -Werror -shared -static '-Wl,--no-insert-timestamp' (Join-Path $PSScriptRoot 'RandomPlacement.cpp') -o (Join-Path $build 'DecorativeRandomiser.dll') -luser32
if ($LASTEXITCODE -ne 0) { throw 'DLL compilation failed' }
Write-Host "Built $build\DecorativeRandomiser.dll"
