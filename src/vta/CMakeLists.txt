vedam_module(vedam_vta
    vta.c
    class_uuid.c
    codec.cc
    struct_codec.cc
    inproc/ops.c
    mq/ops.c
    grpc/http.cc
    grpc/ops.c
    grpc_uds/ops.c
    connect/ops.c
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/v1/types.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/system/v1/system.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/identity/v1/identity.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/storage/v1/storage.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/security/v1/security.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/wasm/v1/wasm.pb.cc
)
target_include_directories(vedam_vta PRIVATE ${CMAKE_SOURCE_DIR}/include/vti)
target_link_libraries(vedam_vta PUBLIC
    vedam_via
    vedam_lock
    vedam_ds
    vedam_util
    vedam_platform
    protobuf::libprotobuf
)
add_dependencies(vedam_vta vedam_codegen)
