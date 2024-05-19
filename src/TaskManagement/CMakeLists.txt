file(GLOB_RECURSE all_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
add_library(TaskManagement ${all_sources})