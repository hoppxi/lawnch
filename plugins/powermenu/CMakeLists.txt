cmake_minimum_required(VERSION 3.17)
project(powermenu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(powermenu SHARED
  powermenu.cpp
)

# this will be replaced once the header is installed system wide
target_include_directories(powermenu PRIVATE
  ../
)

set_target_properties(powermenu PROPERTIES
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS powermenu
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lawnch/plugins
)
