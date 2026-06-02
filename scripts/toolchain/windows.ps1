# Toolchain env setup for the `worse` build on Windows (PowerShell). DOT-SOURCE this, then run cmake:
#
#   . scripts\toolchain\windows.ps1
#   cmake --preset release
#
# It sets $env:CC/$env:CXX (LLVM clang -- the C++20 modules need a modern clang) and $env:VCPKG_ROOT
# (your vcpkg checkout). The CMakePresets read $env{VCPKG_ROOT} for the toolchain file and $CXX/$CC
# for the compiler, so nothing machine-specific lives in the repo. Anything you set yourself wins.
#
# NOTE: the dot ('. ') prefix is required so the env vars persist in your session; running the
# script normally would set them only in a child scope.

# --- compiler: LLVM clang --------------------------------------------------------------------
if (-not $env:CXX) {
    $clangpp = Get-Command clang++ -ErrorAction SilentlyContinue
    if ($clangpp) {
        $env:CXX = $clangpp.Source
        $clang = Get-Command clang -ErrorAction SilentlyContinue
        if ($clang) { $env:CC = $clang.Source }
    }
    elseif (Test-Path "C:\Program Files\LLVM\bin\clang++.exe") {
        $env:CXX = "C:\Program Files\LLVM\bin\clang++.exe"
        $env:CC  = "C:\Program Files\LLVM\bin\clang.exe"
    }
    else {
        Write-Warning "toolchain/windows: LLVM clang not found — install it (e.g. 'winget install LLVM.LLVM') or set `$env:CXX."
    }
}

# --- vcpkg root: existing env -> CI var -> probe common checkouts ----------------------------
if (-not $env:VCPKG_ROOT) {
    if ($env:VCPKG_INSTALLATION_ROOT) {
        $env:VCPKG_ROOT = $env:VCPKG_INSTALLATION_ROOT
    }
    else {
        foreach ($cand in @("C:\vcpkg", "C:\dev\vcpkg", "$env:USERPROFILE\vcpkg", "$env:USERPROFILE\dev\vcpkg")) {
            if (Test-Path (Join-Path $cand "scripts\buildsystems\vcpkg.cmake")) {
                $env:VCPKG_ROOT = $cand
                break
            }
        }
    }
}

if ($env:VCPKG_ROOT) { Write-Host "toolchain/windows: VCPKG_ROOT=$($env:VCPKG_ROOT)" }
else { Write-Warning "toolchain/windows: VCPKG_ROOT not set and no vcpkg checkout found — set `$env:VCPKG_ROOT." }
if ($env:CXX) { Write-Host "toolchain/windows: CXX=$($env:CXX)" }
