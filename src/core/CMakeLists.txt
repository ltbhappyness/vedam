vedam_module(vedam_node
    core.c
    ${CMAKE_SOURCE_DIR}/src/va/system.c
    ${CMAKE_SOURCE_DIR}/src/va/identity.c
    ${CMAKE_SOURCE_DIR}/src/va/storage.c
    ${CMAKE_SOURCE_DIR}/src/va/security.c
    ${CMAKE_SOURCE_DIR}/src/va/wasm.c
    ${CMAKE_SOURCE_DIR}/src/via/meta.c
    ${CMAKE_SOURCE_DIR}/src/via/system.c
    ${CMAKE_SOURCE_DIR}/src/via/identity.c
    ${CMAKE_SOURCE_DIR}/src/via/storage.c
    ${CMAKE_SOURCE_DIR}/src/via/security.c
    ${CMAKE_SOURCE_DIR}/src/via/wasm.c
    ${CMAKE_SOURCE_DIR}/src/lib/system.c
    ${CMAKE_SOURCE_DIR}/src/lib/identity.c
    ${CMAKE_SOURCE_DIR}/src/lib/storage.c
    ${CMAKE_SOURCE_DIR}/src/lib/security.c
    ${CMAKE_SOURCE_DIR}/src/lib/wasm.c
)
target_include_directories(vedam_node PRIVATE ${CMAKE_SOURCE_DIR}/src/via)
target_link_libraries(vedam_node PUBLIC vedam_vta vedam_util)
add_dependencies(vedam_node vedam_codegen)
