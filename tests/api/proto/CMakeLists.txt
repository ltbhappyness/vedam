vedam_test(vedam_api_proto_test NODE
    SOURCES message_test.cc
    LIBS vedam
)
target_include_directories(vedam_api_proto_test PRIVATE ${CMAKE_SOURCE_DIR}/include/vti)
