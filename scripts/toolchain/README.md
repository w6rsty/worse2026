# Toolchain setup

`CMakePresets.json` is machine-agnostic: it reads the vcpkg toolchain from `$env{VCPKG_ROOT}` and
the compiler from the standard `CXX`/`CC` environment variables. These scripts set those env vars
for you with sensible per-platform detection, then you run the usual preset.

## Usage

**macOS / Ubuntu** (source into your current shell):

```sh
source scripts/toolchain/macos.sh     # or: source scripts/toolchain/ubuntu.sh
cmake --preset release                # debug / relwithdebinfo also work
cmake --build build/release
```

**Windows** (PowerShell — note the leading dot, which keeps the env vars in your session):

```powershell
. scripts\toolchain\windows.ps1
cmake --preset release
cmake --build build/release
```

The scripts honor anything you already exported (`CC`/`CXX`/`VCPKG_ROOT`), so to override just set
it yourself before sourcing. They print what they resolved and warn (without aborting) if a
compiler or vcpkg checkout can't be found.

## Prerequisites

- **vcpkg** checked out somewhere; either export `VCPKG_ROOT=/path/to/vcpkg` or keep it in a common
  location the script probes (`~/dev/Cpp/vcpkg`, `~/vcpkg`, `/opt/vcpkg`, `C:\vcpkg`, …).
- A **clang** new enough for C++20 modules:
  - macOS: `brew install llvm` (Apple's clang is not sufficient).
  - Ubuntu: `sudo apt-get install -y clang lld libc++-dev libc++abi-dev` (clang ≥ 18 recommended).
    Modules on Linux usually also want `-stdlib=libc++` — add it via `CMAKE_CXX_FLAGS` or a
    `CMakeUserPresets.json` if your default libstdc++ can't build them.
  - Windows: `winget install LLVM.LLVM`.
- **Ninja** (`brew install ninja` / `apt-get install ninja-build` / `winget install Ninja-build.Ninja`).

## Alternative: `CMakeUserPresets.json`

`CMakeUserPresets.json` is git-ignored. If you'd rather not source a script each time, drop your
machine-specific paths there instead:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "release",
      "inherits": "release",
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": "/abs/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake",
        "CMAKE_CXX_COMPILER": "/abs/path/to/clang++"
      }
    }
  ]
}
```

(CI uses the scripts — see `.github/workflows/container-ci.yml`.)
