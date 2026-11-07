param([string]$Zig = 'zig', [string]$Python = 'python')
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    & $Python tools/make_fixtures.py
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic fixture generation failed.' }
    New-Item -ItemType Directory -Force tests/build | Out-Null
    foreach ($test in @('test_watch','test_launcher','test_rebar_page','test_full_setup')) {
        $executable = "tests/build/$test.exe"
        & $Zig cc -O0 -Wall -Wextra -Wno-unused-function -fshort-wchar -I src "tests/$test.c" -o $executable
        if ($LASTEXITCODE -ne 0) { throw "Test build failed: $test" }
        & "./$executable"
        if ($LASTEXITCODE -ne 0) { throw "Test failed: $test" }
    }
} finally { Pop-Location }
