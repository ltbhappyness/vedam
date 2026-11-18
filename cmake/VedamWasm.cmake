# Class E product. emcc is required. The host archives and vedamd are not built here.
# VEDAM_EMCC is a path passed on the cmake command line. Otherwise emcc is taken from PATH.

if(VEDAM_EMCC)
    set(vedam_emcc "${VEDAM_EMCC}")
else()
    find_program(vedam_emcc_path emcc)
    set(vedam_emcc "${vedam_emcc_path}")
endif()
if(NOT vedam_emcc OR NOT EXISTS "${vedam_emcc}")
    message(FATAL_ERROR "emcc was not found")
endif()

foreach(mod vedam_platform vedam_ds vedam_util vedam_lock vedam_lib vedam_via vedam_va vedam_vta vedam_core)
    set_target_properties(${mod} PROPERTIES EXCLUDE_FROM_ALL TRUE)
endforeach()
if(TARGET wwlock)
    set_target_properties(wwlock wwlock_shared PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# No POSIX threads in this module. thread.c and futex.c stay on the host build.
set(vedam_wasm_c
    ${CMAKE_SOURCE_DIR}/src/platform/posix/memory.c
    ${CMAKE_SOURCE_DIR}/src/platform/posix/string.c
    ${CMAKE_SOURCE_DIR}/src/platform/posix/sleep.c
    ${CMAKE_SOURCE_DIR}/src/platform/posix/env.c
    ${CMAKE_SOURCE_DIR}/src/platform/posix/process.c
    ${CMAKE_SOURCE_DIR}/src/ds/list/list.c
    ${CMAKE_SOURCE_DIR}/src/ds/stack/stack.c
    ${CMAKE_SOURCE_DIR}/src/ds/queue/queue.c
    ${CMAKE_SOURCE_DIR}/src/ds/hash/hash.c
    ${CMAKE_SOURCE_DIR}/src/ds/kv/singleton.c
    ${CMAKE_SOURCE_DIR}/src/ds/kv/index.c
    ${CMAKE_SOURCE_DIR}/src/ds/kv/hashkv.c
    ${CMAKE_SOURCE_DIR}/src/util/attr.c
    ${CMAKE_SOURCE_DIR}/src/lib/system.c
    ${CMAKE_SOURCE_DIR}/src/lib/identity.c
    ${CMAKE_SOURCE_DIR}/src/lib/storage.c
    ${CMAKE_SOURCE_DIR}/src/lib/security.c
    ${CMAKE_SOURCE_DIR}/src/lib/contract.c
    ${CMAKE_SOURCE_DIR}/src/lib/plugin.c
    ${CMAKE_SOURCE_DIR}/src/via/classes.c
    ${CMAKE_SOURCE_DIR}/src/via/dispatch.c
    ${CMAKE_SOURCE_DIR}/src/via/meta.c
    ${CMAKE_SOURCE_DIR}/src/via/verb.c
    ${CMAKE_SOURCE_DIR}/src/va/system.c
    ${CMAKE_SOURCE_DIR}/src/va/identity.c
    ${CMAKE_SOURCE_DIR}/src/va/storage.c
    ${CMAKE_SOURCE_DIR}/src/va/security.c
    ${CMAKE_SOURCE_DIR}/src/va/contract.c
    ${CMAKE_SOURCE_DIR}/src/va/plugin.c
    ${CMAKE_SOURCE_DIR}/src/vta/vta.c
    ${CMAKE_SOURCE_DIR}/src/vta/class_uuid.c
    ${CMAKE_SOURCE_DIR}/src/vta/inproc/ops.c
    ${CMAKE_SOURCE_DIR}/src/core/core.c
)

if(VEDAM_ENABLE_MQ)
    list(APPEND vedam_wasm_c
        ${CMAKE_SOURCE_DIR}/src/platform/posix/mq.c
        ${CMAKE_SOURCE_DIR}/src/vta/mq/ops.c
    )
endif()
if(VEDAM_ENABLE_GRPC OR VEDAM_ENABLE_GRPC_UDS)
    list(APPEND vedam_wasm_c ${CMAKE_SOURCE_DIR}/src/vta/grpc/http.cc)
endif()
if(VEDAM_ENABLE_GRPC)
    list(APPEND vedam_wasm_c ${CMAKE_SOURCE_DIR}/src/vta/grpc/ops.c)
endif()
if(VEDAM_ENABLE_GRPC_UDS)
    list(APPEND vedam_wasm_c ${CMAKE_SOURCE_DIR}/src/vta/grpc_uds/ops.c)
endif()
if(VEDAM_ENABLE_CONNECT)
    list(APPEND vedam_wasm_c ${CMAKE_SOURCE_DIR}/src/vta/connect/ops.c)
endif()
if(VEDAM_ENABLE_WWLOCK)
    list(APPEND vedam_wasm_c
        ${CMAKE_SOURCE_DIR}/src/lock/lock.c
        ${CMAKE_SOURCE_DIR}/src/lock/store.c
        ${CMAKE_SOURCE_DIR}/third_party/wwlock/src/wwlock.c
        ${CMAKE_SOURCE_DIR}/third_party/wwlock/src/wwlock_futex.c
    )
else()
    list(APPEND vedam_wasm_c ${CMAKE_SOURCE_DIR}/src/lock/nolock.c)
endif()

set(vedam_wasm_flags
    -O2
    -DVEDAM_OS_LINUX
    -I ${CMAKE_SOURCE_DIR}/include
    -include ${VEDAM_CONFIG_HEADER}
    -sEXPORTED_FUNCTIONS=_vedam_init
    --no-entry
)
if(VEDAM_ENABLE_WWLOCK)
    list(APPEND vedam_wasm_flags -I ${CMAKE_SOURCE_DIR}/third_party/wwlock/include)
endif()

set(VEDAM_WASM "${CMAKE_BINARY_DIR}/libvedam.wasm")
# -o names the JS glue. emcc writes libvedam.wasm beside it. A .wasm -o would
# turn on standalone wasm, which a page cannot instantiate.
add_custom_command(
    OUTPUT ${VEDAM_WASM}
    COMMAND ${vedam_emcc} ${vedam_wasm_flags} ${vedam_wasm_c} -o ${CMAKE_BINARY_DIR}/libvedam.js
    DEPENDS ${vedam_wasm_c} ${VEDAM_CONFIG_HEADER}
    COMMENT "Building libvedam.wasm"
    VERBATIM
)
add_custom_target(vedam_wasm ALL DEPENDS ${VEDAM_WASM})
add_dependencies(vedam_wasm vedam_codegen)
install(FILES ${VEDAM_WASM} DESTINATION lib)
