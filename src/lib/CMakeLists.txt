vedam_module(vedam_lib
    system.c
    identity.c
    storage.c
    security.c
    wasm.c
)
target_link_libraries(vedam_lib PUBLIC vedam_lock vedam_util vedam_platform)
add_dependencies(vedam_lib vedam_codegen)
