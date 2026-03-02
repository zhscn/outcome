cmake_minimum_required(VERSION 3.16)
project(outcome VERSION 0.1.0 LANGUAGES CXX)

add_library(outcome INTERFACE)
add_library(outcome::outcome ALIAS outcome)

target_include_directories(
  outcome INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(
  DIRECTORY include/outcome
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp"
)

install(TARGETS outcome EXPORT outcomeTargets)

install(
  EXPORT outcomeTargets
  FILE outcomeTargets.cmake
  NAMESPACE outcome::
  DESTINATION lib/cmake/outcome
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/outcomeConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/outcomeConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/outcomeConfig.cmake
  INSTALL_DESTINATION lib/cmake/outcome
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/outcomeConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/outcomeConfigVersion.cmake
  DESTINATION lib/cmake/outcome
)
