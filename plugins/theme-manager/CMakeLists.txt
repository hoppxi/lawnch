cmake_minimum_required(VERSION 3.17)
project(theme-manager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LawnchPluginApi REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(INIH REQUIRED inih)

add_library(theme-manager SHARED
    theme_manager.cpp
)

target_include_directories(theme-manager PRIVATE
    ../
    ${INIH_INCLUDE_DIRS}
)

target_link_libraries(theme-manager PRIVATE 
    stdc++fs
    ${INIH_LIBRARIES}
	Lawnch::LawnchPluginApi
)

set_target_properties(theme-manager PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS theme-manager
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins
)
