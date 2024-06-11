# list all source files and headers
file(
  GLOB_RECURSE all_module_cpp
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.cpp")
list(FILTER all_module_cpp EXCLUDE REGEX ".*Test\\.cpp$")

file(
  GLOB_RECURSE all_module_headers
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.h")
list(FILTER all_module_headers EXCLUDE REGEX ".*Test\\.h$")

add_library(Utils ${all_module_cpp} ${all_module_headers})

add_executable(UtilsTest UtilsTest.cpp)
target_link_libraries(UtilsTest Utils)
add_test(NAME UtilsTest COMMAND ${CMAKE_CURRENT_BINARY_DIR}/UtilsTest)
