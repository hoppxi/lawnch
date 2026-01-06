cmake_minimum_required(VERSION 3.17)
project(google LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_library(google SHARED
  google.cpp
)

target_include_directories(google PRIVATE
  ../
)

target_link_libraries(google PRIVATE 
  CURL::libcurl
)

set_target_properties(google PROPERTIES
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS google
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins
)