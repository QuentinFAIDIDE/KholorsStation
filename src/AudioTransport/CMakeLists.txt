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

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# list all source files
file(
  GLOB_RECURSE all_module_cpp
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.cpp")
file(
  GLOB_RECURSE all_module_headers
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.h")

# define the library that includes both our code and protobuf generated one
add_library(
  AudioTransport ${at_grpc_srcs} ${at_proto_hdrs} ${at_proto_srcs}
                 ${at_grpc_hdrs} ${all_module_cpp} ${all_module_headers})

# link the library with the required libs
target_link_libraries(AudioTransport ${_REFLECTION} ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF})

# add library testing target
add_executable(AudioTransportTest AudioTransportTest.cpp)
target_link_libraries(
  AudioTransportTest
  AudioTransport
  absl::flags
  absl::flags_parse
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_test(NAME AudioTransportTest
         COMMAND ${CMAKE_CURRENT_BINARY_DIR}/AudioTransportTest)
