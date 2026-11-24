cmake_minimum_required(VERSION 3.25)
project(TamanduaBandeira VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 3.4 CONFIG REQUIRED)

add_executable(tamandua_bandeira src/main.cpp)
target_link_libraries(tamandua_bandeira PRIVATE SDL3::SDL3)
target_compile_definitions(tamandua_bandeira PRIVATE SDL_MAIN_HANDLED)

if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
    target_link_libraries(tamandua_bandeira PRIVATE PkgConfig::EGL)

    include(dkp-asset-folder)
    set(game_romfs "${CMAKE_CURRENT_BINARY_DIR}/romfs")
    dkp_add_asset_target(tamandua_bandeira_romfs "${game_romfs}")
    file(GLOB_RECURSE game_assets CONFIGURE_DEPENDS "${game_romfs}/assets/*")
    foreach(asset IN LISTS game_assets)
        set_property(TARGET tamandua_bandeira_romfs APPEND PROPERTY DKP_ASSET_FILES "${asset}")
    endforeach()

    nx_generate_nacp(OUTPUT tamandua_bandeira.nacp NAME "Tamandua Bandeira" AUTHOR "Projeto independente" VERSION "${PROJECT_VERSION}")
    nx_create_nro(tamandua_bandeira ROMFS tamandua_bandeira_romfs
        ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.jpg" NACP tamandua_bandeira.nacp)
endif()
