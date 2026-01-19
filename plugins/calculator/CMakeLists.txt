cmake_minimum_required(VERSION 3.17)
project(calculator LANGUAGES CXX)


find_package(LawnchPluginApi REQUIRED)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(calculator SHARED
	calculator.cpp
)

target_link_libraries(calculator PRIVATE
	Lawnch::LawnchPluginApi
)

set_target_properties(calculator PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS calculator
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins
)
