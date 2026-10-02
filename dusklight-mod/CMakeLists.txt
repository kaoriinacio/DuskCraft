cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED ENV{CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type")
endif ()

project(tpcraft VERSION 0.0.1 LANGUAGES CXX)

# Versao do Dusklight para buildar contra: tag de release ou SHA.
# Use -DDUSKLIGHT_DIR=/caminho/dusklight para apontar para um checkout existente.
# Este projeto deve ficar preso a uma tag valida do upstream do Dusklight.
if (NOT DUSKLIGHT_VERSION)
    set(DUSKLIGHT_VERSION "v2.0.3")
endif ()
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchDusklight.cmake")
add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)

configure_file(mod.json.in "${CMAKE_CURRENT_BINARY_DIR}/mod.json" @ONLY NEWLINE_STYLE LF)

# Mod nativo. Adicionar `webgpu` em FEATURES quando for desenhar blocos (GfxService).
add_mod(tpcraft
    FEATURES game fmt webgpu
        SOURCES src/mod.cpp src/link.cpp
        MOD_JSON "${CMAKE_CURRENT_BINARY_DIR}/mod.json"
        RES_DIR res
)

# Teste standalone da ponte (nao depende do Dusklight): cmake --build . --target link_selftest
add_executable(link_selftest EXCLUDE_FROM_ALL src/link_selftest.cpp src/link.cpp)
target_include_directories(link_selftest PRIVATE src)
