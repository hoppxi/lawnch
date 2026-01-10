cmake_minimum_required(VERSION 3.17)

project(lawnch LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-protocols wayland-egl)
pkg_check_modules(CAIRO REQUIRED cairo pango pangocairo)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
pkg_check_modules(PANGOCAIRO REQUIRED pangocairo)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
pkg_check_modules(WLR_PROTOCOLS REQUIRED wlr-protocols)
pkg_check_modules(LIBRSVG REQUIRED librsvg-2.0)
pkg_check_modules(INIH REQUIRED inih)

find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

find_program(WAYLAND_SCANNER_EXECUTABLE wayland-scanner)
if(NOT WAYLAND_SCANNER_EXECUTABLE)
	message(FATAL_ERROR "wayland-scanner not found.")
endif()

pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
pkg_get_variable(WLR_PROTO_DIR wlr-protocols pkgdatadir)

set(XDG_SHELL_PROTOCOL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
set(XDG_SHELL_CLIENT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h")
set(XDG_SHELL_PRIVATE_CODE "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c")

set(WLR_LAYER_SHELL_PROTOCOL_XML "${WLR_PROTO_DIR}/unstable/wlr-layer-shell-unstable-v1.xml")
set(WLR_LAYER_SHELL_CLIENT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-client-protocol.h")
set(WLR_LAYER_SHELL_PRIVATE_CODE "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.c")

add_custom_command(
	OUTPUT ${XDG_SHELL_CLIENT_HEADER}
	COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${XDG_SHELL_PROTOCOL_XML} ${XDG_SHELL_CLIENT_HEADER}
	DEPENDS ${XDG_SHELL_PROTOCOL_XML}
)
add_custom_command(
	OUTPUT ${XDG_SHELL_PRIVATE_CODE}
	COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${XDG_SHELL_PROTOCOL_XML} ${XDG_SHELL_PRIVATE_CODE}
	DEPENDS ${XDG_SHELL_PROTOCOL_XML}
)

add_custom_command(
	OUTPUT ${WLR_LAYER_SHELL_CLIENT_HEADER}
	COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${WLR_LAYER_SHELL_PROTOCOL_XML} ${WLR_LAYER_SHELL_CLIENT_HEADER}
	DEPENDS ${WLR_LAYER_SHELL_PROTOCOL_XML}
)
add_custom_command(
	OUTPUT ${WLR_LAYER_SHELL_PRIVATE_CODE}
	COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${WLR_LAYER_SHELL_PROTOCOL_XML} ${WLR_LAYER_SHELL_PRIVATE_CODE}
	DEPENDS ${WLR_LAYER_SHELL_PROTOCOL_XML}
)

add_library(wayland_protocols STATIC
	${WLR_LAYER_SHELL_PRIVATE_CODE}
	${WLR_LAYER_SHELL_CLIENT_HEADER}
	${XDG_SHELL_PRIVATE_CODE}
	${XDG_SHELL_CLIENT_HEADER}
)
set_target_properties(wayland_protocols PROPERTIES LINKER_LANGUAGE C)
target_include_directories(wayland_protocols PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_library(dl INTERFACE)
target_link_libraries(dl INTERFACE dl)

add_executable(lawnch
	src/main.cpp
	src/config/config_manager.cpp
	src/helpers/gfx.cpp
	src/helpers/fs.cpp
	src/helpers/string.cpp
	src/helpers/process.cpp
	src/helpers/logger.cpp
	src/search/search.cpp
	src/search/plugin_manager.cpp
	src/search/plugin_adapter.cpp
	src/search/apps.cpp
	src/search/bins.cpp
	src/window/window.cpp
	src/window/handlers.cpp
	src/window/icon.cpp
	src/window/render.cpp
	src/window/input.cpp
)

target_include_directories(lawnch PRIVATE
	src
	${CMAKE_CURRENT_BINARY_DIR}
	${WAYLAND_INCLUDE_DIRS}
	${CAIRO_INCLUDE_DIRS}
	${PANGOCAIRO_INCLUDE_DIRS}
	${XKBCOMMON_INCLUDE_DIRS}
	${CURL_INCLUDE_DIRS}
	${LIBRSVG_INCLUDE_DIRS}
	${INIH_INCLUDE_DIRS}
)

target_link_libraries(lawnch PRIVATE
	wayland_protocols
	${WAYLAND_LIBRARIES}
	${CAIRO_LIBRARIES}
	${PANGOCAIRO_LIBRARIES}
	${HARFBUZZ_LIBRARIES}
	${XKBCOMMON_LIBRARIES}
	nlohmann_json::nlohmann_json
	${CURL_LIBRARIES}
	${LIBRSVG_LIBRARIES}
	${INIH_LIBRARIES}
	dl
	rt
)

target_compile_options(lawnch PRIVATE
	-Os
	-DNDEBUG
	-fdata-sections
	-ffunction-sections
)

target_link_options(lawnch PRIVATE
	-Wl,--gc-sections
	-Wl,--as-needed
	-s
)

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported)
if(lto_supported)
	set_property(TARGET lawnch PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
