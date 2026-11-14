vedam_test(vedam_via_test NODE
    SOURCES via_test.cc ../support/node_stub.c
    LIBS vedam_via vedam_lib vedam_lock vedam_ds vedam_util vedam_platform
)
