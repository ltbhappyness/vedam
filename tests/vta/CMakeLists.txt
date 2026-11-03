vedam_test(vedam_vta_codec_test NODE
    SOURCES legacy_entry.cc legacy_codec.c
    LIBS vedam
)
vedam_test(vedam_vta_http_test NODE
    SOURCES legacy_grpc_read.cc
    LIBS vedam
)
target_include_directories(vedam_vta_http_test PRIVATE ${CMAKE_SOURCE_DIR}/include/vti)
