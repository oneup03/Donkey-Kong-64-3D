# Donkey Kong 64: Recompiled for the Nintendo 3DS.
#
#   cmake -S platform/3ds -B build-3ds -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=<rt64-3ds>/cmake/devkitARM-3ds.cmake
#
# The recompiled sources (RecompiledFuncs/, RecompiledPatches/, rsp/) are
# generated on the host first; see the Makefile beside this file.
cmake_minimum_required(VERSION 3.20)
project(DK64Recompiled3DS LANGUAGES C CXX ASM)

set(DK64_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(RT64_3DS_DIR "${DK64_ROOT}/lib/rt64-3ds" CACHE PATH "rt64-3ds checkout")
set(RT64_3DS_NMR_DIR "${DK64_ROOT}/lib/N64ModernRuntime")
add_subdirectory("${RT64_3DS_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/rt64-3ds")

# The generated game code. Implicit declarations are how the generator refers
# to host functions; dk64_host_protos.h gives them real prototypes so the
# 64-bit arguments are passed correctly on a 32-bit CPU.
file(GLOB FUNC_SOURCES "${DK64_ROOT}/RecompiledFuncs/*.c")
add_library(RecompiledFuncs STATIC ${FUNC_SOURCES} "${DK64_ROOT}/RecompiledFuncs/lookup.cpp")
target_include_directories(RecompiledFuncs PRIVATE
    "${DK64_ROOT}/RecompiledFuncs" "${CMAKE_CURRENT_LIST_DIR}"
    "${RT64_3DS_NMR_DIR}/N64Recomp/include" "${RT64_3DS_NMR_DIR}/ultramodern/include" "${RT64_3DS_NMR_DIR}/librecomp/include")
target_compile_options(RecompiledFuncs PRIVATE
    -O2 -fno-strict-aliasing -w -Wno-implicit-function-declaration -Wno-error=implicit-function-declaration
    -include "${CMAKE_CURRENT_LIST_DIR}/dk64_host_protos.h")

add_library(RecompiledPatches STATIC
    "${DK64_ROOT}/RecompiledPatches/patches.c"
    "${DK64_ROOT}/RecompiledPatches/patches_bin.c")
target_include_directories(RecompiledPatches PRIVATE
    "${DK64_ROOT}/RecompiledPatches" "${CMAKE_CURRENT_LIST_DIR}"
    "${RT64_3DS_NMR_DIR}/N64Recomp/include" "${RT64_3DS_NMR_DIR}/ultramodern/include" "${RT64_3DS_NMR_DIR}/librecomp/include")
# No force-include here: the patches' own funcs.h declares load_dk64_overlay
# with the (rdram, ctx) shape, which would conflict.
target_compile_options(RecompiledPatches PRIVATE
    -O2 -fno-strict-aliasing -w -Wno-implicit-function-declaration -Wno-error=implicit-function-declaration)

add_executable(DK64Recompiled3DS
    main_3ds.cpp
    recomp_api_3ds.cpp
    "${DK64_ROOT}/rsp/n_aspMain.cpp"
    "${DK64_ROOT}/src/game/rom_decompression.cpp"
    "${DK64_ROOT}/src/main/register_overlays.cpp"
    "${DK64_ROOT}/src/main/register_patches.cpp"
)
target_include_directories(DK64Recompiled3DS PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}" "${DK64_ROOT}/include" "${DK64_ROOT}"
    "${DK64_ROOT}/RecompiledFuncs" "${DK64_ROOT}/RecompiledPatches"
    "${RT64_3DS_NMR_DIR}/N64Recomp/include" "${RT64_3DS_NMR_DIR}/thirdparty")
target_compile_options(DK64Recompiled3DS PRIVATE -O2 -Wno-unused-parameter)
# The audio microcode runs scalar on the ARM11 and is the hottest file.
set_source_files_properties("${DK64_ROOT}/rsp/n_aspMain.cpp" PROPERTIES COMPILE_OPTIONS "-O3;-fno-strict-aliasing")
target_link_libraries(DK64Recompiled3DS PRIVATE
    recomp_3ds rt64_3ds RecompiledPatches RecompiledFuncs ultramodern librecomp citro3d ctru m)
target_link_options(DK64Recompiled3DS PRIVATE "-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/DK64Recompiled3DS.map")

set(DK64_TITLE "DK64 ReKongpiled")
set(DK64_DESCRIPTION "Donkey Kong 64 Recompiled for New 3DS")
add_3dsx(DK64Recompiled3DS TITLE "${DK64_TITLE}" DESCRIPTION "${DK64_DESCRIPTION}"
         ICON "${CMAKE_CURRENT_LIST_DIR}/icon.png")
include("${RT64_3DS_DIR}/cmake/Cia.cmake")
# The banner: a bunch of golden bananas and DK's "Ooh, banana!", made by
# the tools in a separate repository (3ds-banner-tools); only their output is here.
add_cia(DK64Recompiled3DS RSF "${RT64_3DS_DIR}/cia/template.rsf" TITLE "${DK64_TITLE}" PRODUCT "DK64-3DS" UNIQUEID 0xF534E
        ICON "${CMAKE_CURRENT_LIST_DIR}/icon.png" DESCRIPTION "${DK64_DESCRIPTION}"
        BANNER_CGFX "${CMAKE_CURRENT_LIST_DIR}/cia/banner.cgfx" BANNER_WAV "${CMAKE_CURRENT_LIST_DIR}/cia/banner.wav")
