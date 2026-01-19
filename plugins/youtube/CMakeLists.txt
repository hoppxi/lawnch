cmake_minimum_required(VERSION 3.17)
project(youtube LANGUAGES CXX)


find_package(LawnchPluginApi REQUIRED)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_library(youtube SHARED
	youtube.cpp
)

target_link_libraries(youtube PRIVATE
	Lawnch::LawnchPluginApi
)

target_link_libraries(youtube PRIVATE 
	CURL::libcurl
)

set_target_properties(youtube PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS youtube
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins
)
