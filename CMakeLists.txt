cmake_minimum_required(VERSION 3.16)
project(outcome VERSION 0.1.0 LANGUAGES CXX)

add_library(outcome INTERFACE)
add_library(outcome::outcome ALIAS outcome)

target_include_directories(
  outcome INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_library(outcome_std_error_code INTERFACE)
add_library(outcome::std_error_code ALIAS outcome_std_error_code)
target_compile_definitions(outcome_std_error_code INTERFACE OUTCOME_ENABLE_STD_ERROR_CODE)
target_link_libraries(outcome_std_error_code INTERFACE outcome)

add_library(outcome_exception INTERFACE)
add_library(outcome::exception ALIAS outcome_exception)
target_compile_definitions(outcome_exception INTERFACE OUTCOME_ENABLE_EXCEPTION)
target_link_libraries(outcome_exception INTERFACE outcome)

install(
  DIRECTORY include/
  DESTINATION include/outcome
  FILES_MATCHING PATTERN "*.hpp"
)

install(
  TARGETS outcome outcome_std_error_code outcome_exception
  EXPORT outcomeTargets
)

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
