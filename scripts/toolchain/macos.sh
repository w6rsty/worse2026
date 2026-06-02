#!/usr/bin/env bash
#
# Toolchain env setup for the `worse` build on macOS. SOURCE this (do not execute), then run cmake:
#
#   source scripts/toolchain/macos.sh
#   cmake --preset release
#
# It exports CC/CXX (Homebrew LLVM clang -- Apple's clang lacks the C++20 module support this
# project needs) and VCPKG_ROOT (your vcpkg checkout). The CMakePresets read $env{VCPKG_ROOT} for
# the toolchain file and $CXX/$CC for the compiler, so nothing machine-specific lives in the repo.
#
# Anything you export yourself beforehand wins -- e.g. `export CXX=/path/to/clang++` to override.

# Warn (don't abort) if executed instead of sourced -- exports wouldn't persist then.
(return 0 2>/dev/null) || echo "toolchain/macos: run me with 'source', not as a program — exports won't stick otherwise." >&2

# --- compiler: Homebrew LLVM clang ------------------------------------------------------------
if [ -z "${CXX:-}" ]; then
    _llvm="$(brew --prefix llvm 2>/dev/null || true)"
    if [ -n "$_llvm" ] && [ -x "$_llvm/bin/clang++" ]; then
        export CC="$_llvm/bin/clang"
        export CXX="$_llvm/bin/clang++"
    else
        echo "toolchain/macos: Homebrew LLVM clang not found — run 'brew install llvm' (Apple clang may not build the C++20 modules)." >&2
    fi
    unset _llvm
fi

# --- vcpkg root: existing env -> CI var -> probe common checkouts -----------------------------
if [ -z "${VCPKG_ROOT:-}" ]; then
    if [ -n "${VCPKG_INSTALLATION_ROOT:-}" ]; then
        export VCPKG_ROOT="$VCPKG_INSTALLATION_ROOT"
    else
        for _cand in "$HOME/dev/Cpp/vcpkg" "$HOME/vcpkg" "$HOME/.vcpkg" "/opt/vcpkg" "/usr/local/vcpkg"; do
            if [ -f "$_cand/scripts/buildsystems/vcpkg.cmake" ]; then
                export VCPKG_ROOT="$_cand"
                break
            fi
        done
        unset _cand
    fi
fi

if [ -z "${VCPKG_ROOT:-}" ]; then
    echo "toolchain/macos: VCPKG_ROOT not set and no vcpkg checkout found — 'export VCPKG_ROOT=/path/to/vcpkg'." >&2
else
    echo "toolchain/macos: VCPKG_ROOT=$VCPKG_ROOT"
fi
[ -n "${CXX:-}" ] && echo "toolchain/macos: CXX=$CXX"
