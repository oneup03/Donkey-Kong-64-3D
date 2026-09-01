@echo off
REM Unified setup + configure + build script for DK64Recompiled (Windows).
REM
REM   build.bat                    - bring everything up to date, then build Release
REM   build.bat clean              - delete the CMake cache first, forcing a fresh configure
REM   build.bat regen              - also re-run N64Recomp + RSPRecomp on the .toml files
REM                                  (use when us.toml / n_aspMain.toml / DK64Syms change)
REM   build.bat rom                - only (re)generate donkeykong64.decompressed.us.z64
REM   build.bat tools              - only rebuild N64Recomp.exe / RSPRecomp.exe
REM   build.bat <cmd> Debug        - any of the above against a Debug build tree
REM
REM Configs: Release (default), Debug, RelWithDebInfo. Each gets its own build
REM directory so switching between them does not force a reconfigure.
REM
REM Use the `clean` form if a CMakeLists change leaves the cache in a bad state
REM (symptom: regen picks the wrong lld-link and fails with an x86/x64 machine
REM type mismatch).
setlocal enabledelayedexpansion

REM Repo root = this script's directory (strip trailing backslash).
set "REPO=%~dp0"
if "%REPO:~-1%"=="\" set "REPO=%REPO:~0,-1%"

REM --- Parse args ---------------------------------------------------------
set "CMD=%~1"
if "%CMD%"=="" set "CMD=build"
set "CONFIG=%~2"
if "%CONFIG%"=="" set "CONFIG=Release"

REM Allow `build.bat Debug` as shorthand for `build.bat build Debug`.
if /i "%CMD%"=="Release"        ( set "CONFIG=Release"        & set "CMD=build" )
if /i "%CMD%"=="Debug"          ( set "CONFIG=Debug"          & set "CMD=build" )
if /i "%CMD%"=="RelWithDebInfo" ( set "CONFIG=RelWithDebInfo" & set "CMD=build" )

if /i "%CONFIG%"=="Release"        ( set "BLD=%REPO%\build-cmake" & goto :config_ok )
if /i "%CONFIG%"=="Debug"          ( set "BLD=%REPO%\build-cmake-debug" & goto :config_ok )
if /i "%CONFIG%"=="RelWithDebInfo" ( set "BLD=%REPO%\build-cmake-relwithdebinfo" & goto :config_ok )
echo [build] ERROR: unknown config "%CONFIG%" ^(expected Release, Debug or RelWithDebInfo^).
exit /b 1
:config_ok

REM --- Tool locations -----------------------------------------------------
REM MIPS-capable clang for the `patches` library. The clang bundled with Visual
REM Studio is built without MIPS targets, so patches\Makefile needs a separate
REM LLVM. Expected as a sibling checkout of this repo:
REM   <parent>\portable-llvm\LLVM-19.1.3-Windows-X64
REM from https://github.com/llvm/llvm-project/releases (19.1.3, Windows x64).
set "PATCHES_LLVM=%REPO%\..\portable-llvm\LLVM-19.1.3-Windows-X64\bin"
set "N64RECOMP_SRC=%REPO%\lib\N64ModernRuntime\N64Recomp"
REM Kept outside the submodule so `git status` doesn't report it as dirty.
set "N64RECOMP_BLD=%REPO%\build-tools\N64Recomp"
set "CHOCO_BIN=C:\ProgramData\chocolatey\bin"

if /i "%CMD%"=="clean" (
    echo [build] Clean requested - removing CMake cache in %BLD%...
    if exist "%BLD%\CMakeCache.txt" del /q "%BLD%\CMakeCache.txt"
    if exist "%BLD%\CMakeFiles" rmdir /s /q "%BLD%\CMakeFiles"
)

REM --- Visual Studio environment ------------------------------------------
REM vswhere locates whichever edition is installed rather than assuming Community.
set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [build] ERROR: vswhere.exe not found. Install Visual Studio 2022 with
    echo [build]        "Desktop development with C++", "C++ Clang Compiler for Windows"
    echo [build]        and "C++ CMake tools for Windows".
    exit /b 1
)
set "VSPATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if "%VSPATH%"=="" (
    echo [build] ERROR: no Visual Studio install with the x64 C++ toolset was found.
    exit /b 1
)

REM VsDevCmd (called by vcvars64) shells out to a bare `vswhere`, so the
REM Installer directory has to be on PATH or it spews "not recognized" errors.
for %%d in ("%VSWHERE%") do set "PATH=%%~dpd;%PATH%"
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo [build] ERROR: vcvars64.bat failed.
    exit /b 1
)

REM Prepend chocolatey (make.exe, used by patches\Makefile) and the x64 LLVM
REM shipped with VS (NOT the 32-bit ...\Llvm\bin) so any in-build CMake regen
REM keeps using the x64 lld-link and does not trip the machine-type mismatch.
set "PATH=%CHOCO_BIN%;%VSPATH%\VC\Tools\Llvm\x64\bin;%PATH%"

where make >nul 2>&1
if errorlevel 1 (
    echo [build] ERROR: make.exe not on PATH. patches\Makefile needs it.
    echo [build]        Install with: choco install make
    exit /b 1
)

if not exist "%PATCHES_LLVM%\clang.exe" (
    echo [build] ERROR: patches LLVM not found at "%PATCHES_LLVM%"
    echo [build]        Expected sibling layout: %REPO%\..\portable-llvm\LLVM-19.1.3-Windows-X64
    echo [build]        Extract portable LLVM 19.1.3 there or adjust PATCHES_LLVM in build.bat.
    exit /b 1
)

REM --- Decompressed ROM ----------------------------------------------------
REM us.toml and n_aspMain.toml both read donkeykong64.decompressed.us.z64.
set "ROM=%REPO%\donkeykong64.decompressed.us.z64"
set "DECOMP_SCRIPT=%REPO%\lib\dk64_decomp\tools\generate_decompressed_rom.py"
set "ROM_SHA1=cf806ff2603640a748fca5026ded28802f1f4a50"

if /i "%CMD%"=="rom" if exist "%ROM%" del /q "%ROM%"
if exist "%ROM%" goto :rom_done

echo [build] Generating donkeykong64.decompressed.us.z64...
if not exist "%DECOMP_SCRIPT%" (
    echo [build] ERROR: %DECOMP_SCRIPT% is missing. Fetch submodules with:
    echo [build]        git submodule update --init --recursive
    exit /b 1
)

REM Pick the repo-root .z64 that is 32 MiB and hashes as NTSC-U 1.0, so the
REM ROM can keep whatever name it was dumped under.
set "BASEROM="
for %%f in ("%REPO%\*.z64") do (
    if %%~zf EQU 33554432 (
        for /f "skip=1 tokens=1" %%h in ('certutil -hashfile "%%f" SHA1') do (
            if /i "%%h"=="%ROM_SHA1%" if not defined BASEROM set "BASEROM=%%~ff"
        )
    )
)
if not defined BASEROM (
    echo [build] ERROR: no .z64 in the repo root has sha1 %ROM_SHA1%.
    echo [build]        Put an NTSC-U 1.0 Donkey Kong 64 ROM there. See BUILDING.md step 3.
    exit /b 1
)
echo [build] Using "%BASEROM%"

REM generate_decompressed_rom.py takes no arguments - it reads baserom.us.z64
REM from the current directory and writes baserom.us.decompressed.z64 beside
REM it, so stage a copy under those names in a scratch dir.
set "PY=python"
where python >nul 2>&1 || set "PY=py"
set "ROMTMP=%TEMP%\dk64-decomp-%RANDOM%"
mkdir "%ROMTMP%"
copy /Y "%BASEROM%" "%ROMTMP%\baserom.us.z64" >nul
pushd "%ROMTMP%"
%PY% "%DECOMP_SCRIPT%"
set "PRC=!ERRORLEVEL!"
popd
if not "!PRC!"=="0" (
    rmdir /s /q "%ROMTMP%"
    echo [build] ERROR: generate_decompressed_rom.py failed ^(exit code !PRC!^).
    exit /b 1
)
move /Y "%ROMTMP%\baserom.us.decompressed.z64" "%ROM%" >nul
rmdir /s /q "%ROMTMP%"
if not exist "%ROM%" (
    echo [build] ERROR: donkeykong64.decompressed.us.z64 was not produced.
    exit /b 1
)
:rom_done

if /i "%CMD%"=="rom" exit /b 0

REM --- Recompiler executables ----------------------------------------------
if /i "%CMD%"=="tools" (
    if exist "%REPO%\N64Recomp.exe" del /q "%REPO%\N64Recomp.exe"
    if exist "%REPO%\RSPRecomp.exe" del /q "%REPO%\RSPRecomp.exe"
)

if exist "%REPO%\N64Recomp.exe" if exist "%REPO%\RSPRecomp.exe" goto :tools_done

echo [build] Building N64Recomp / RSPRecomp from lib\N64ModernRuntime\N64Recomp...
if not exist "%N64RECOMP_SRC%\CMakeLists.txt" (
    echo [build] ERROR: %N64RECOMP_SRC% is empty. Fetch submodules with:
    echo [build]        git submodule update --init --recursive
    exit /b 1
)
REM These are host tools unrelated to the game build, so plain MSVC is fine and
REM avoids dragging the game's clang-cl/Ninja settings into them.
cmake -S "%N64RECOMP_SRC%" -B "%N64RECOMP_BLD%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1
cmake --build "%N64RECOMP_BLD%" --config Release --target N64RecompCLI RSPRecomp
if errorlevel 1 exit /b 1
copy /Y "%N64RECOMP_BLD%\Release\N64Recomp.exe" "%REPO%\N64Recomp.exe" >nul
copy /Y "%N64RECOMP_BLD%\Release\RSPRecomp.exe" "%REPO%\RSPRecomp.exe" >nul
:tools_done

if /i "%CMD%"=="tools" exit /b 0

REM --- Run the recompilers -------------------------------------------------
if /i "%CMD%"=="regen" (
    if exist "%REPO%\RecompiledFuncs" (
        echo [build] regen requested - removing RecompiledFuncs...
        rmdir /s /q "%REPO%\RecompiledFuncs"
    )
    if exist "%REPO%\rsp\n_aspMain.cpp" del /q "%REPO%\rsp\n_aspMain.cpp"
)

if not exist "%REPO%\RecompiledFuncs\funcs.h" (
    echo [build] Running N64Recomp on us.toml...
    pushd "%REPO%"
    .\N64Recomp.exe us.toml
    set "NRC=!ERRORLEVEL!"
    popd
    REM N64Recomp returns non-zero if any single function fails to recompile
    REM (e.g. an unhandled MIPS instruction) but still emits valid C for the
    REM other 4500+ functions, so funcs.h existing is the real success signal -
    REM the same check the upstream N64Recomp ports rely on.
    if not exist "%REPO%\RecompiledFuncs\funcs.h" (
        echo [build] ERROR: N64Recomp did not generate funcs.h ^(exit code !NRC!^).
        exit /b 1
    )
    if not "!NRC!"=="0" (
        echo [build] WARN: N64Recomp exited !NRC! - some functions may have failed to recompile.
    )
)

if not exist "%REPO%\rsp" mkdir "%REPO%\rsp"
if not exist "%REPO%\rsp\n_aspMain.cpp" (
    echo [build] Running RSPRecomp on n_aspMain.toml...
    pushd "%REPO%"
    .\RSPRecomp.exe n_aspMain.toml
    set "RRC=!ERRORLEVEL!"
    popd
    if not exist "%REPO%\rsp\n_aspMain.cpp" (
        echo [build] ERROR: RSPRecomp did not generate rsp\n_aspMain.cpp ^(exit code !RRC!^).
        exit /b 1
    )
)

REM --- vcpkg ---------------------------------------------------------------
REM CMakeLists.txt points VCPKG_ROOT at lib\vcpkg; vcpkg.json pulls in curl for
REM the mod loader.
if not exist "%REPO%\lib\vcpkg\vcpkg.exe" (
    echo [build] Bootstrapping vcpkg...
    if not exist "%REPO%\lib\vcpkg\bootstrap-vcpkg.bat" (
        echo [build] ERROR: lib\vcpkg is empty. Fetch submodules with:
        echo [build]        git submodule update --init --recursive
        exit /b 1
    )
    call "%REPO%\lib\vcpkg\bootstrap-vcpkg.bat" -disableMetrics
    if errorlevel 1 exit /b 1
)

REM --- Configure -----------------------------------------------------------
if not exist "%BLD%\CMakeCache.txt" (
    echo [build] Configuring CMake ^(%CONFIG%^)...
    REM -Xclang -fexceptions is what the upstream Windows CI passes; clang-cl
    REM otherwise builds the curl-backed mod loader without exception support.
    cmake -G Ninja -DCMAKE_BUILD_TYPE=%CONFIG% ^
        -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl ^
        -DPATCHES_C_COMPILER="%PATCHES_LLVM%\clang.exe" ^
        -DPATCHES_LD="%PATCHES_LLVM%\ld.lld.exe" ^
        -DCMAKE_CXX_FLAGS="-Xclang -fexceptions -Xclang -fcxx-exceptions" ^
        -S "%REPO%" -B "%BLD%"
    if errorlevel 1 exit /b 1
)

REM --- Build ---------------------------------------------------------------
echo [build] Building DK64Recompiled ^(%CONFIG%^)...
cmake --build "%BLD%" --target DK64Recompiled --config %CONFIG%
if errorlevel 1 exit /b 1

echo.
echo [build] Output: %BLD%\DK64Recompiled.exe
exit /b 0
