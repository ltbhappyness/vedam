vedam_module(vedam_via
    classes.c
    dispatch.c
    meta.c
    verb.c
)
target_link_libraries(vedam_via PUBLIC vedam_lib vedam_lock vedam_util vedam_platform)
add_dependencies(vedam_via vedam_codegen)
