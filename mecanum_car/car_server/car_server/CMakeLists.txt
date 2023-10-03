# Original File from gRPC
# https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/CMakeLists.txt

cmake_minimum_required(VERSION 3.8)

project(CarServer C CXX)

include(../cmake/common.cmake)

# Proto file
get_filename_component(mc_proto "../../protos/mecanum.proto" ABSOLUTE)
get_filename_component(mc_proto_path "${mc_proto}" PATH)

# Generated sources
set(mc_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/mecanum.pb.cc")
set(mc_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/mecanum.pb.h")
set(mc_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/mecanum.grpc.pb.cc")
set(mc_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/mecanum.grpc.pb.h")
add_custom_command(
      OUTPUT "${mc_proto_srcs}" "${mc_proto_hdrs}" "${mc_grpc_srcs}" "${mc_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${mc_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${mc_proto}"
      DEPENDS "${mc_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# mc_grpc_proto
add_library(mc_grpc_proto
  ${mc_grpc_srcs}
  ${mc_grpc_hdrs}
  ${mc_proto_srcs}
  ${mc_proto_hdrs})
target_link_libraries(mc_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# Targets greeter_[async_](client|server)
foreach(_target
  car_server)
  add_executable(${_target} "${_target}.cc")
  target_link_libraries(${_target}
    mc_grpc_proto
    pigpio
    absl::flags
    absl::flags_parse
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()
