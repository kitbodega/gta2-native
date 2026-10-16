cmake_minimum_required(VERSION 3.22.1)
project(gta2native CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Flags del port (mismo set que build_tranche*.ps1).
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fdeclspec -fms-extensions -fshort-wchar -D_CRT_SECURE_NO_WARNINGS -Dswprintf=GTA2_swprintf -Wno-write-strings")

set(ENGINE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../gta2_re/Source)
set(REPO_ROOT      ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../gta2_re)
set(PLATFORM_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/platform)

file(GLOB ENGINE_SOURCES "${ENGINE_SRC_DIR}/*.cpp")

# Modulos excluidos: graficos/audio red (stub en el port) y herramientas PC.
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/Bink\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/dma_video\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/gbh_graphics\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/NetPlay\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/Network_20324\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/ImGuiDebug\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/HookLoader\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/ExePatcher\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/cl_proxy\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/winmain\\.cpp$")

add_library(gta2native SHARED
        jni_native.cpp
        wrap_strcpy.cpp
        ${ENGINE_SOURCES}
        ${PLATFORM_DIR}/wincompat.cpp
        ${PLATFORM_DIR}/wincompat_registry.cpp
        ${PLATFORM_DIR}/mss_stubs.cpp
        ${PLATFORM_DIR}/gles_glue.cpp
        ${PLATFORM_DIR}/engine_backend.cpp)

target_include_directories(gta2native PRIVATE
        ${PLATFORM_DIR}
        ${ENGINE_SRC_DIR}
        ${ENGINE_SRC_DIR}/3rdParty
        ${REPO_ROOT})

find_library(log-lib log)
find_library(z-lib z)
find_library(OpenSLES-lib OpenSLES)
find_library(GLESv2-lib GLESv2)
find_library(EGL-lib EGL)
target_link_libraries(gta2native android ${log-lib} ${z-lib} ${OpenSLES-lib} ${GLESv2-lib} ${EGL-lib} dl m)

# Los fuentes del motor referencian DInput.lib/dxguid.lib via #pragma comment(lib)
# (no existen en Android); ignoramos dependencias y simbolos sin resolver de
# librerias estaticas (resueltos en runtime desde el sistema).
target_link_options(gta2native PRIVATE
        "-Wl,--no-dependent-libraries"
        "-Wl,--unresolved-symbols=ignore-in-object-files"
        "-Wl,--warn-unresolved-symbols"
        "-Wl,--wrap=strcpy"
        "-Wl,--wrap=__strcpy_chk")