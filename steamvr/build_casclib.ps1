$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$source = Join-Path $repoRoot 'deps/CascLib'
$revision = '4971d363e665551ac4142f541e5f2d71f1cda653' # upstream 3.0
if (!(Test-Path -LiteralPath $source)) {
    & git clone --depth 1 --branch 3.0 https://github.com/ladislav-zezula/CascLib.git $source
    if ($LASTEXITCODE -ne 0) { throw 'CascLib source download failed' }
}
if ((& git -C $source rev-parse HEAD).Trim() -ne $revision -or (& git -C $source status --porcelain)) {
    throw 'CascLib sources must be clean and at the pinned upstream 3.0 revision'
}
$cmake = Join-Path $repoRoot 'deps/cmake-4.4.4-windows-x86_64/bin/cmake.exe'
$ninja = Join-Path $repoRoot 'deps/ninja/ninja.exe'
$build = Join-Path $repoRoot 'build-frame/casclib-installer'
& $cmake -S (Join-Path $PSScriptRoot 'casclib') -B $build -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" "-DCMAKE_TOOLCHAIN_FILE=$repoRoot/cmake/windows-llvm-toolchain.cmake" "-DCASCLIB_SOURCE_DIR=$source" '-DCMAKE_BUILD_TYPE=Release'
if ($LASTEXITCODE -ne 0) { throw 'CascLib configure failed' }
& $cmake --build $build --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'CascLib build failed' }
Write-Host "Built $build/CascLib.dll (installer only; no game assets)."
