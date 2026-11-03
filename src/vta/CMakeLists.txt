vedam_module(vedam_vta
    vta.c
    dispatch.c
    attrwire.c
    class_uuid.c
    codec.cc
    inproc/ops.c
    mq/ops.c
    grpc/http.cc
    grpc/ops.c
    grpc_uds/ops.c
    connect/ops.c
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/types.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/system.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/identity.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/storage.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/security.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/wasm.pb.cc
)
target_include_directories(vedam_vta PRIVATE
    ${CMAKE_SOURCE_DIR}/include/vti
    ${CMAKE_SOURCE_DIR}/src/via
    ${CMAKE_SOURCE_DIR}/src/lock
)
target_link_libraries(vedam_vta PUBLIC vedam_lock protobuf::libprotobuf)
add_dependencies(vedam_vta vedam_codegen)
