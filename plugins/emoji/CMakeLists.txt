cmake_minimum_required(VERSION 3.17)
project(emoji LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)

add_library(emoji SHARED
	emoji.cpp
)

target_include_directories(emoji PRIVATE
	../
)

target_link_libraries(emoji PRIVATE nlohmann_json::nlohmann_json)

set_target_properties(emoji PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS emoji
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins
)
