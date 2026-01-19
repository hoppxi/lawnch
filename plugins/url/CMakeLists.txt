cmake_minimum_required(VERSION 3.17)
project(url LANGUAGES CXX)


find_package(LawnchPluginApi REQUIRED)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(url SHARED
	url.cpp
)

target_link_libraries(url PRIVATE
	Lawnch::LawnchPluginApi
)

set_target_properties(url PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS url
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins
)
