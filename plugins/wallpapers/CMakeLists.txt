cmake_minimum_required(VERSION 3.17)
project(wallpapers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(wallpapers SHARED
	wallpapers.cpp
)

target_include_directories(wallpapers PRIVATE
	../
)

set_target_properties(wallpapers PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS wallpapers
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins
)
