cmake_minimum_required(VERSION 3.17)
project(lawnch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

target_include_directories(lawnch PRIVATE src)

target_link_libraries(lawnch PRIVATE
    ${GTK4_LIBRARIES}
    ${GTK4_LAYER_SHELL_LIBRARIES}
    nlohmann_json::nlohmann_json
    ${CURL_LIBRARIES}
)

target_compile_options(lawnch PRIVATE
    ${GTK4_CFLAGS_OTHER}
    ${GTK4_LAYER_SHELL_CFLAGS_OTHER}
)

target_include_directories(lawnch PRIVATE
    ${GTK4_INCLUDE_DIRS}
    ${GTK4_LAYER_SHELL_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS} 
)