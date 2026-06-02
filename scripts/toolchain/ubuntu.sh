#!/usr/bin/env bash
#
# Toolchain env setup for the `worse` build on Ubuntu/Linux. SOURCE this (do not execute), then:
#
#   source scripts/toolchain/ubuntu.sh
#   cmake --preset release
#
# It exports CC/CXX (a recent clang -- the C++20 modules need a modern toolchain) and VCPKG_ROOT.
# The CMakePresets read $env{VCPKG_ROOT} for the toolchain file and $CXX/$CC for the compiler.
# Anything you export yourself beforehand wins.
#
# NOTE: C++20 modules with clang on Linux usually also want libc++ (`-stdlib=libc++`,
# `libc++-dev`/`libc++abi-dev` installed). This script only selects the compiler; add the flag
# via CMAKE_CXX_FLAGS or a CMakeUserPresets.json if your libstdc++ doesn't build the modules.

(return 0 2>/dev/null) || echo "toolchain/ubuntu: run me with 'source', not as a program — exports won't stick otherwise." >&2

# --- compiler: newest available clang --------------------------------------------------------
if [ -z "${CXX:-}" ]; then
    for _v in 22 21 20 19 18 ""; do
        _sfx="${_v:+-$_v}"
        if command -v "clang++${_sfx}" >/dev/null 2>&1; then
            export CXX="$(command -v "clang++${_sfx}")"
            command -v "clang${_sfx}" >/dev/null 2>&1 && export CC="$(command -v "clang${_sfx}")"
            break
        fi
    done
    unset _v _sfx
    [ -z "${CXX:-}" ] && echo "toolchain/ubuntu: no clang++ found — install clang (>=18 recommended for modules), e.g. 'sudo apt-get install -y clang lld libc++-dev libc++abi-dev'." >&2
fi

# --- vcpkg root: existing env -> CI var -> probe common checkouts -----------------------------
if [ -z "${VCPKG_ROOT:-}" ]; then
    if [ -n "${VCPKG_INSTALLATION_ROOT:-}" ]; then
        export VCPKG_ROOT="$VCPKG_INSTALLATION_ROOT"
    else
        for _cand in "$HOME/vcpkg" "$HOME/.vcpkg" "$HOME/dev/vcpkg" "/opt/vcpkg" "/usr/local/vcpkg"; do
            if [ -f "$_cand/scripts/buildsystems/vcpkg.cmake" ]; then
                export VCPKG_ROOT="$_cand"
                break
            fi
        done
        unset _cand
    fi
fi

if [ -z "${VCPKG_ROOT:-}" ]; then
    echo "toolchain/ubuntu: VCPKG_ROOT not set and no vcpkg checkout found — 'export VCPKG_ROOT=/path/to/vcpkg'." >&2
else
    echo "toolchain/ubuntu: VCPKG_ROOT=$VCPKG_ROOT"
fi
[ -n "${CXX:-}" ] && echo "toolchain/ubuntu: CXX=$CXX"
