file(
  GLOB_RECURSE all_sources
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.cpp")
add_library(TaskManagement ${all_sources})

add_executable(TaskingManagerTest TaskingManagerTest.cpp)
target_link_libraries(TaskingManagerTest TaskManagement
                      nlohmann_json::nlohmann_json spdlog fmt)
add_test(NAME TaskingManagerTest
         COMMAND ${CMAKE_CURRENT_BINARY_DIR}/TaskingManagerTest)
