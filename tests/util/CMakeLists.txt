vedam_test(vedam_util_test
    SOURCES attr_test.cc
    LIBS vedam_util vedam_platform
)
vedam_test(vedam_util_legacy_test NODE
    SOURCES legacy_entry.cc legacy_util.c
    LIBS vedam
)
