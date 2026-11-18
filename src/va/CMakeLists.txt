vedam_module(vedam_va
    system.c
    identity.c
    storage.c
    security.c
    contract.c
    plugin.c
)
target_link_libraries(vedam_va PUBLIC vedam_via)
add_dependencies(vedam_va vedam_codegen)
