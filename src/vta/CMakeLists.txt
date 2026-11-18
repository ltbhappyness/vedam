set(vedam_vta_sources
    vta.c
    class_uuid.c
    codec.cc
    struct_codec.cc
    inproc/ops.c
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/v1/types.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/system/v1/system.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/identity/v1/identity.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/storage/v1/storage.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/security/v1/security.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/contract/v1/contract.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/plugin/v1/plugin.pb.cc
)
if(VEDAM_ENABLE_MQ)
    list(APPEND vedam_vta_sources mq/ops.c)
endif()
if(VEDAM_ENABLE_GRPC OR VEDAM_ENABLE_GRPC_UDS)
    list(APPEND vedam_vta_sources grpc/http.cc)
endif()
if(VEDAM_ENABLE_GRPC)
    list(APPEND vedam_vta_sources grpc/ops.c)
endif()
if(VEDAM_ENABLE_GRPC_UDS)
    list(APPEND vedam_vta_sources grpc_uds/ops.c)
endif()
if(VEDAM_ENABLE_CONNECT)
    list(APPEND vedam_vta_sources connect/ops.c)
endif()

vedam_module(vedam_vta ${vedam_vta_sources})
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
