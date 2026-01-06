cmake_minimum_required(VERSION 3.17)
project(clipboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(clipboard SHARED
	clipboard.cpp
)

target_include_directories(clipboard PRIVATE
	../
)

set_target_properties(clipboard PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS clipboard
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins
)
