# defining targets that are handy for protobuf execution in this target
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP grpc++)
if(CMAKE_CROSSCOMPILING)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
endif()

# defining paths to proto file
get_filename_component(at_proto "./AudioTransport.proto" ABSOLUTE)
get_filename_component(at_proto_path "${at_proto}" PATH)

# handling source generated from proto file
set(at_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/AudioTransport.pb.cc")
set(at_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/AudioTransport.pb.h")
set(at_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/AudioTransport.grpc.pb.cc")
set(at_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/AudioTransport.grpc.pb.h")
add_custom_command(
  OUTPUT "${at_proto_srcs}" "${at_proto_hdrs}" "${at_grpc_srcs}"
         "${at_grpc_hdrs}"
  COMMAND
    ${_PROTOBUF_PROTOC} ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}" --cpp_out
    "${CMAKE_CURRENT_BINARY_DIR}" -I "${at_proto_path}"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}" "${at_proto}"
  DEPENDS "${at_proto}")

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

# define the library that includes both our code and protobuf generated one
add_library(
  AudioTransport ${at_grpc_srcs} ${at_proto_hdrs} ${at_proto_srcs}
                 ${at_grpc_hdrs} ${all_module_cpp} ${all_module_headers})

# Include generated *.pb.h files
target_include_directories(AudioTransport PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

target_include_directories(AudioTransport
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

# link the library with the required libs
target_link_libraries(AudioTransport TaskManagement ${_REFLECTION}
                      ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})

set_target_properties(AudioTransport PROPERTIES POSITION_INDEPENDENT_CODE ON)

# list all test source files and headers
file(
  GLOB_RECURSE all_test_cpp
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*Test.cpp")

file(
  GLOB_RECURSE all_test_headers
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*Test.h")

# add library testing target
add_executable(AudioTransportTest ${all_test_cpp} ${all_test_headers})
target_link_libraries(
  AudioTransportTest
  AudioTransport
  TaskManagement
  absl::flags
  absl::flags_parse
  spdlog::spdlog
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_test(NAME AudioTransportTest
         COMMAND ${CMAKE_CURRENT_BINARY_DIR}/AudioTransportTest)
