cmake_minimum_required(VERSION 3.17)
project(command LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(command SHARED
	command.cpp
)

target_include_directories(command PRIVATE
	../
)

set_target_properties(command PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS command
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins
)
