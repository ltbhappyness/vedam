vedam_module(vedam_core core.c)
target_link_libraries(vedam_core PUBLIC
    vedam_va
    vedam_vta
    vedam_via
    vedam_lib
    vedam_lock
    vedam_ds
    vedam_util
    vedam_platform
)
add_dependencies(vedam_core vedam_codegen)
