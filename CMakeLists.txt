cmake_minimum_required(VERSION 3.17)
project(lawnch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(GTK4_LAYER_SHELL REQUIRED gtk4-layer-shell-0)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

add_executable(lawnch
    src/main.cpp
    src/window.cpp
    src/search.cpp
    src/utils.cpp
)

target_include_directories(lawnch PRIVATE
    src
    ${GTK4_INCLUDE_DIRS}
    ${GTK4_LAYER_SHELL_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(lawnch PRIVATE
    ${GTK4_LIBRARIES}
    ${GTK4_LAYER_SHELL_LIBRARIES}
    nlohmann_json::nlohmann_json
    ${CURL_LIBRARIES}
)

target_compile_options(lawnch PRIVATE
    -Os
    -DNDEBUG
    -fdata-sections
    -ffunction-sections
)

target_link_options(lawnch PRIVATE
    -Wl,--gc-sections
    -Wl,--as-needed
    -s
)

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported)
if(lto_supported)
    set_property(TARGET lawnch PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
