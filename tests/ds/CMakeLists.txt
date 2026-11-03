vedam_test(vedam_ds_test
    SOURCES ds_test.cc
    LIBS vedam_ds vedam_platform
)
vedam_test(vedam_ds_legacy_test NODE
    SOURCES legacy_entry.cc legacy_ds.c
    LIBS vedam
)
