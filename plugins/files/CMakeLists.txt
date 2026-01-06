cmake_minimum_required(VERSION 3.17)
project(files LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(files SHARED
	files.cpp
)

target_include_directories(files PRIVATE
	../
)

set_target_properties(files PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS files
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins
)
