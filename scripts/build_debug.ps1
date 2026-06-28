param(
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"
cmake -S . -B $BuildDir -A x64 -DCMAKE_BUILD_TYPE=Debug
cmake --build $BuildDir --config Debug
ctest --test-dir $BuildDir -C Debug --output-on-failure
