cmake_minimum_required(VERSION 3.16)
project(vedam C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install into the build tree" FORCE)
endif()

find_package(Protobuf REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(Threads REQUIRED)

set(CODEGEN "${CMAKE_SOURCE_DIR}/tools/codegen")
set(GEN_TREE
  ${CMAKE_SOURCE_DIR}/include/vi/types.h
  ${CMAKE_SOURCE_DIR}/include/vi/vedam.h
  ${CMAKE_SOURCE_DIR}/include/vi/class/system.h
  ${CMAKE_SOURCE_DIR}/include/vi/class/identity.h
  ${CMAKE_SOURCE_DIR}/include/vi/class/storage.h
  ${CMAKE_SOURCE_DIR}/include/vi/class/security.h
  ${CMAKE_SOURCE_DIR}/include/vi/class/wasm.h
  ${CMAKE_SOURCE_DIR}/include/vii/types.h
  ${CMAKE_SOURCE_DIR}/include/vii/class/system.h
  ${CMAKE_SOURCE_DIR}/include/vii/class/identity.h
  ${CMAKE_SOURCE_DIR}/include/vii/class/storage.h
  ${CMAKE_SOURCE_DIR}/include/vii/class/security.h
  ${CMAKE_SOURCE_DIR}/include/vii/class/wasm.h
  ${CMAKE_SOURCE_DIR}/src/via/meta.h
  ${CMAKE_SOURCE_DIR}/src/va/system.c
  ${CMAKE_SOURCE_DIR}/src/va/identity.c
  ${CMAKE_SOURCE_DIR}/src/va/storage.c
  ${CMAKE_SOURCE_DIR}/src/va/security.c
  ${CMAKE_SOURCE_DIR}/src/va/wasm.c
  ${CMAKE_SOURCE_DIR}/src/via/system.c
  ${CMAKE_SOURCE_DIR}/src/via/identity.c
  ${CMAKE_SOURCE_DIR}/src/via/storage.c
  ${CMAKE_SOURCE_DIR}/src/via/security.c
  ${CMAKE_SOURCE_DIR}/src/via/wasm.c
  ${CMAKE_SOURCE_DIR}/src/vta/dispatch.c
  ${CMAKE_SOURCE_DIR}/src/vta/attrwire.c
  ${CMAKE_SOURCE_DIR}/src/vta/codec.cc
  ${CMAKE_SOURCE_DIR}/src/vta/class_uuid.c
  ${CMAKE_SOURCE_DIR}/include/vti/class_uuid.h
  ${CMAKE_SOURCE_DIR}/src/lock/attr_ids.c
  ${CMAKE_SOURCE_DIR}/proto/vedam/types.proto
  ${CMAKE_SOURCE_DIR}/proto/vedam/system.proto
  ${CMAKE_SOURCE_DIR}/proto/vedam/identity.proto
  ${CMAKE_SOURCE_DIR}/proto/vedam/storage.proto
  ${CMAKE_SOURCE_DIR}/proto/vedam/security.proto
  ${CMAKE_SOURCE_DIR}/proto/vedam/wasm.proto
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/types.pb.cc
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/types.pb.h
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/system.pb.cc
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/system.pb.h
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/identity.pb.cc
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/identity.pb.h
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/storage.pb.cc
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/storage.pb.h
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/security.pb.cc
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/security.pb.h
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/wasm.pb.cc
  ${CMAKE_SOURCE_DIR}/include/vti/vedam/wasm.pb.h
)

add_custom_command(
  OUTPUT ${GEN_TREE}
  COMMAND ${Python3_EXECUTABLE} ${CODEGEN}/generate.py --root ${CMAKE_SOURCE_DIR}
  DEPENDS
    ${CODEGEN}/generate.py
    ${CODEGEN}/infra/context.py
    ${CODEGEN}/infra/copyright.py
    ${CODEGEN}/infra/schema.py
    ${CODEGEN}/infra/emit.py
    ${CODEGEN}/infra/render.py
    ${CODEGEN}/schema/vedam.yaml
    ${CODEGEN}/src/types.yaml
    ${CODEGEN}/src/classes/system.yaml
    ${CODEGEN}/src/classes/identity.yaml
    ${CODEGEN}/src/classes/storage.yaml
    ${CODEGEN}/src/classes/security.yaml
    ${CODEGEN}/src/classes/wasm.yaml
    ${CODEGEN}/templates/proto/types.proto.j2
    ${CODEGEN}/templates/proto/class.proto.j2
    ${CODEGEN}/templates/c/vi_types.h.j2
    ${CODEGEN}/templates/c/vi_vedam.h.j2
    ${CODEGEN}/templates/c/vii_types.h.j2
    ${CODEGEN}/templates/c/via_meta.h.j2
    ${CODEGEN}/templates/c/class_vi.h.j2
    ${CODEGEN}/templates/c/class_vii.h.j2
    ${CODEGEN}/templates/c/class_va.c.j2
    ${CODEGEN}/templates/c/class_via.c.j2
    ${CODEGEN}/templates/c/dispatch.c.j2
    ${CODEGEN}/templates/c/attr_ids.c.j2
    ${CODEGEN}/templates/c/attrwire.c.j2
    ${CODEGEN}/templates/c/class_uuid.h.j2
    ${CODEGEN}/templates/c/class_uuid.c.j2
    ${CODEGEN}/templates/cxx/codec.cc.j2
  COMMENT "Generating headers, va/via, proto, and vti into the source tree"
)

add_custom_target(vedam_codegen DEPENDS ${GEN_TREE})

add_library(vedam
  src/core/core.c
  src/lock/wwlock.c
  src/lock/wwlock_futex.c
  src/lock/lock.c
  src/lock/store.c
  src/platform/posix/futex.c
  src/ds/list/list.c
  src/ds/stack/stack.c
  src/ds/queue/queue.c
  src/ds/hash/hash.c
  src/ds/kv/singleton.c
  src/ds/kv/index.c
  src/ds/kv/hashkv.c
  src/lib/system.c
  src/lib/identity.c
  src/lib/storage.c
  src/lib/security.c
  src/lib/wasm.c
  src/platform/posix/memory.c
  src/platform/posix/string.c
  src/platform/posix/thread.c
  src/platform/posix/sleep.c
  src/platform/posix/mq.c
  src/platform/posix/env.c
  src/platform/posix/process.c
  src/util/attr.c
  src/va/system.c
  src/va/identity.c
  src/va/storage.c
  src/va/security.c
  src/va/wasm.c
  src/via/meta.c
  src/via/system.c
  src/via/identity.c
  src/via/storage.c
  src/via/security.c
  src/via/wasm.c
  src/vta/vta.c
  src/lock/attr_ids.c
  src/vta/dispatch.c
  src/vta/attrwire.c
  src/vta/class_uuid.c
  src/vta/codec.cc
  src/vta/inproc/ops.c
  src/vta/mq/ops.c
  src/vta/grpc/http.cc
  src/vta/grpc/ops.c
  src/vta/grpc_uds/ops.c
  src/vta/connect/ops.c
  include/vti/vedam/types.pb.cc
  include/vti/vedam/system.pb.cc
  include/vti/vedam/identity.pb.cc
  include/vti/vedam/storage.pb.cc
  include/vti/vedam/security.pb.cc
  include/vti/vedam/wasm.pb.cc
)
add_dependencies(vedam vedam_codegen)

target_include_directories(vedam
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_SOURCE_DIR}/include/vti
    ${CMAKE_SOURCE_DIR}/src/lock
    ${CMAKE_SOURCE_DIR}/src/via
)

target_link_libraries(vedam
  PUBLIC
    protobuf::libprotobuf
    Threads::Threads
    rt
)

add_executable(vedamd src/vedamd/main.c)
target_link_libraries(vedamd PRIVATE vedam)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/vi DESTINATION include)
install(TARGETS vedam vedamd
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

enable_testing()

add_executable(test_ds tests/test_ds.c)
target_link_libraries(test_ds PRIVATE vedam)
add_test(NAME test_ds COMMAND test_ds)

add_executable(test_util tests/test_util.c)
target_link_libraries(test_util PRIVATE vedam)
add_test(NAME test_util COMMAND test_util)

add_executable(test_system tests/test_system.c)
target_link_libraries(test_system PRIVATE vedam)
add_test(NAME test_system COMMAND test_system)

add_executable(test_lock_transport tests/test_lock_transport.c)
target_link_libraries(test_lock_transport PRIVATE vedam)
add_test(NAME test_lock_transport COMMAND test_lock_transport)

add_executable(test_grpc_read tests/test_grpc_read.cc)
target_link_libraries(test_grpc_read PRIVATE vedam)
target_include_directories(test_grpc_read PRIVATE ${CMAKE_SOURCE_DIR}/include/vti)
add_test(NAME test_grpc_read COMMAND test_grpc_read)

add_executable(test_verticals tests/test_verticals.c)
target_link_libraries(test_verticals PRIVATE vedam)
add_test(NAME test_verticals COMMAND test_verticals)

add_executable(test_codec_r8 tests/test_codec_r8.c)
target_link_libraries(test_codec_r8 PRIVATE vedam)
add_test(NAME test_codec_r8 COMMAND test_codec_r8)

set_tests_properties(test_system test_grpc_read test_lock_transport test_verticals test_codec_r8
  PROPERTIES RESOURCE_LOCK vedam_singleton)

add_executable(example_read_system examples/read_system.c)
target_link_libraries(example_read_system PRIVATE vedam)
