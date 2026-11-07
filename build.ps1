param([string]$Zig = 'zig')
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force bin | Out-Null
    $common = @('cc','-target','x86_64-uefi','-ffreestanding','-fshort-wchar','-fno-stack-protector','-mno-red-zone','-nostdlib','-O2','-Wall','-Wextra')
    foreach ($tool in @('HiiReadOnly','HiiWatch','OpenSetup','RebarPage','FullSetup')) {
        $arguments = $common + @("src/$tool.c", '-o', "bin/$tool.efi")
        if ($tool -eq 'HiiWatch') { $arguments += '-Wl,--subsystem,efi_boot_service_driver' }
        & $Zig @arguments
        if ($LASTEXITCODE -ne 0) { throw "Build failed: $tool" }
    }
    $lines = Get-ChildItem bin/*.efi | Sort-Object Name | ForEach-Object { "$((Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower())  $($_.Name)" }
    [IO.File]::WriteAllLines((Join-Path $PSScriptRoot 'bin/SHA256SUMS.txt'), $lines)
    Write-Output 'Built all five x64 EFI tools.'
} finally { Pop-Location }
