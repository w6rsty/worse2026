#!/usr/bin/env bash
#
# Configure the worse build (UE-style "generate project files"): set up the per-machine
# toolchain env, then run the CMake preset against engine/. This only CONFIGURES — build
# afterwards with your IDE or:  cmake --build engine/build/<preset>
#
# Usage:  ./generate.sh [preset]      # preset defaults to "debug" (release / relwithdebinfo / asan / lto)
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
preset="${1:-debug}"

# Sets CC/CXX (brew llvm) + VCPKG_ROOT; the presets read those. Anything you exported wins.
source "$here/scripts/toolchain/macos.sh"

# Presets are discovered from the source dir, so configure from inside engine/.
( cd "$here/engine" && cmake --preset "$preset" )

echo
echo "Configured engine/build/$preset. Build it with:"
echo "    cmake --build engine/build/$preset"
