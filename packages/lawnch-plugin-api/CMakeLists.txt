cmake_minimum_required(VERSION 3.17)
project(lawnch-plugin-api LANGUAGES CXX)

add_library(LawnchPluginApi INTERFACE)
add_library(Lawnch::PluginApi ALIAS LawnchPluginApi)

include(GNUInstallDirs)

target_include_directories(LawnchPluginApi INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/lawnch>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/lawnch>
)

install(TARGETS LawnchPluginApi
  EXPORT LawnchPluginApiTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/lawnch
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT LawnchPluginApiTargets
  FILE LawnchPluginApiTargets.cmake
  NAMESPACE Lawnch::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LawnchPluginApi
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "LawnchPluginApiConfigVersion.cmake"
  VERSION 0.1.1
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/LawnchPluginApiConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/LawnchPluginApiConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LawnchPluginApi
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/LawnchPluginApiConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/LawnchPluginApiConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LawnchPluginApi
)
