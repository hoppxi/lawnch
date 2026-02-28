cmake_minimum_required(VERSION 3.17)
project(lawnch LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(PkgConfig REQUIRED)

pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-protocols wayland-egl)

find_path(BLEND2D_INCLUDE_DIR NAMES blend2d.h PATH_SUFFIXES blend2d)
find_library(BLEND2D_LIBRARY NAMES blend2d)

if(NOT BLEND2D_INCLUDE_DIR OR NOT BLEND2D_LIBRARY)
  message(FATAL_ERROR "Blend2D not found via find_path/find_library")
endif()

message(STATUS "Blend2D Include: ${BLEND2D_INCLUDE_DIR}")
message(STATUS "Blend2D Library: ${BLEND2D_LIBRARY}")

set(BLEND2D_INCLUDE_DIRS ${BLEND2D_INCLUDE_DIR})
set(BLEND2D_LIBRARIES ${BLEND2D_LIBRARY})
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
pkg_check_modules(WLR_PROTOCOLS REQUIRED wlr-protocols)
find_path(NANOSVG_INCLUDE_DIR NAMES nanosvg.h PATH_SUFFIXES nanosvg)
pkg_check_modules(INIH REQUIRED inih)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
find_package(LawnchPluginApi REQUIRED)

find_program(WAYLAND_SCANNER_EXECUTABLE wayland-scanner)
if(NOT WAYLAND_SCANNER_EXECUTABLE)
  message(FATAL_ERROR "wayland-scanner not found.")
endif()

pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
pkg_get_variable(WLR_PROTO_DIR wlr-protocols pkgdatadir)

set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
set(XDG_SHELL_CLIENT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h")
set(XDG_SHELL_PRIVATE_CODE "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c")

set(WLR_LAYER_SHELL_XML "${WLR_PROTO_DIR}/unstable/wlr-layer-shell-unstable-v1.xml")
set(WLR_LAYER_SHELL_CLIENT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-client-protocol.h")
set(WLR_LAYER_SHELL_PRIVATE_CODE "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.c")

add_custom_command(
  OUTPUT ${XDG_SHELL_CLIENT_HEADER}
  COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${XDG_SHELL_XML} ${XDG_SHELL_CLIENT_HEADER}
  DEPENDS ${XDG_SHELL_XML}
)
add_custom_command(
  OUTPUT ${XDG_SHELL_PRIVATE_CODE}
  COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${XDG_SHELL_XML} ${XDG_SHELL_PRIVATE_CODE}
  DEPENDS ${XDG_SHELL_XML}
)

add_custom_command(
  OUTPUT ${WLR_LAYER_SHELL_CLIENT_HEADER}
  COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${WLR_LAYER_SHELL_XML} ${WLR_LAYER_SHELL_CLIENT_HEADER}
  DEPENDS ${WLR_LAYER_SHELL_XML}
)
add_custom_command(
  OUTPUT ${WLR_LAYER_SHELL_PRIVATE_CODE}
  COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${WLR_LAYER_SHELL_XML} ${WLR_LAYER_SHELL_PRIVATE_CODE}
  DEPENDS ${WLR_LAYER_SHELL_XML}
)

add_library(wayland_protocols STATIC
  ${WLR_LAYER_SHELL_CLIENT_HEADER}
  ${WLR_LAYER_SHELL_PRIVATE_CODE}
  ${XDG_SHELL_CLIENT_HEADER}
  ${XDG_SHELL_PRIVATE_CODE}
)
set_target_properties(wayland_protocols PROPERTIES LINKER_LANGUAGE C)
target_include_directories(wayland_protocols PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB_RECURSE SOURCES
  src/*.cpp
  src/**/*.cpp
)

add_executable(lawnch ${SOURCES})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(lawnch PRIVATE
    -O3
    -flto
    -fno-plt
    -ffast-math
  )
  target_link_options(lawnch PRIVATE
    -flto
    -Wl,--as-needed
    -Wl,-O1
  )
endif()

target_include_directories(lawnch PRIVATE
  src
  ${CMAKE_CURRENT_BINARY_DIR}
  ${WAYLAND_INCLUDE_DIRS}
  ${BLEND2D_INCLUDE_DIRS}
  ${XKBCOMMON_INCLUDE_DIRS}
  ${NANOSVG_INCLUDE_DIR}
  ${INIH_INCLUDE_DIRS}
  ${FONTCONFIG_INCLUDE_DIRS}
)
target_link_libraries(lawnch PRIVATE
  Lawnch::LawnchPluginApi
  wayland_protocols
  ${WAYLAND_LIBRARIES}
  ${BLEND2D_LIBRARIES}
  ${XKBCOMMON_LIBRARIES}
  ${INIH_LIBRARIES}
  ${FONTCONFIG_LIBRARIES}
  dl
  rt
)

install(TARGETS lawnch
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

file(GLOB_RECURSE THEME_FILES "${CMAKE_CURRENT_SOURCE_DIR}/themes/*.ini")
install(FILES ${THEME_FILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/lawnch/themes
)
