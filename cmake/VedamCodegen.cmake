# Regenerate committed sources when the YAML or templates change.

set(CODEGEN "${CMAKE_SOURCE_DIR}/tools/codegen")
set(GEN_TREE
    ${CMAKE_SOURCE_DIR}/include/vi/types.h
    ${CMAKE_SOURCE_DIR}/include/vi/vedam.h
    ${CMAKE_SOURCE_DIR}/include/vi/class/system.h
    ${CMAKE_SOURCE_DIR}/include/vi/class/identity.h
    ${CMAKE_SOURCE_DIR}/include/vi/class/storage.h
    ${CMAKE_SOURCE_DIR}/include/vi/class/security.h
    ${CMAKE_SOURCE_DIR}/include/vi/class/wasm.h
    ${CMAKE_SOURCE_DIR}/include/vii/types.h
    ${CMAKE_SOURCE_DIR}/include/vii/class/system.h
    ${CMAKE_SOURCE_DIR}/include/vii/class/identity.h
    ${CMAKE_SOURCE_DIR}/include/vii/class/storage.h
    ${CMAKE_SOURCE_DIR}/include/vii/class/security.h
    ${CMAKE_SOURCE_DIR}/include/vii/class/wasm.h
    ${CMAKE_SOURCE_DIR}/src/via/meta.h
    ${CMAKE_SOURCE_DIR}/src/va/system.c
    ${CMAKE_SOURCE_DIR}/src/va/identity.c
    ${CMAKE_SOURCE_DIR}/src/va/storage.c
    ${CMAKE_SOURCE_DIR}/src/va/security.c
    ${CMAKE_SOURCE_DIR}/src/va/wasm.c
    ${CMAKE_SOURCE_DIR}/src/via/system.c
    ${CMAKE_SOURCE_DIR}/src/via/identity.c
    ${CMAKE_SOURCE_DIR}/src/via/storage.c
    ${CMAKE_SOURCE_DIR}/src/via/security.c
    ${CMAKE_SOURCE_DIR}/src/via/wasm.c
    ${CMAKE_SOURCE_DIR}/src/vta/dispatch.c
    ${CMAKE_SOURCE_DIR}/src/vta/attrwire.c
    ${CMAKE_SOURCE_DIR}/src/vta/codec.cc
    ${CMAKE_SOURCE_DIR}/src/vta/class_uuid.c
    ${CMAKE_SOURCE_DIR}/include/vti/class_uuid.h
    ${CMAKE_SOURCE_DIR}/src/lock/attr_ids.c
    ${CMAKE_SOURCE_DIR}/proto/vedam/types.proto
    ${CMAKE_SOURCE_DIR}/proto/vedam/system.proto
    ${CMAKE_SOURCE_DIR}/proto/vedam/identity.proto
    ${CMAKE_SOURCE_DIR}/proto/vedam/storage.proto
    ${CMAKE_SOURCE_DIR}/proto/vedam/security.proto
    ${CMAKE_SOURCE_DIR}/proto/vedam/wasm.proto
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/types.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/types.pb.h
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/system.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/system.pb.h
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/identity.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/identity.pb.h
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/storage.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/storage.pb.h
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/security.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/security.pb.h
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/wasm.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/wasm.pb.h
)

add_custom_command(
    OUTPUT ${GEN_TREE}
    COMMAND ${Python3_EXECUTABLE} ${CODEGEN}/generate.py --root ${CMAKE_SOURCE_DIR}
    DEPENDS
        ${CODEGEN}/generate.py
        ${CODEGEN}/infra/context.py
        ${CODEGEN}/infra/copyright.py
        ${CODEGEN}/infra/schema.py
        ${CODEGEN}/infra/emit.py
        ${CODEGEN}/infra/render.py
        ${CODEGEN}/schema/vedam.yaml
        ${CODEGEN}/src/types.yaml
        ${CODEGEN}/src/classes/system.yaml
        ${CODEGEN}/src/classes/identity.yaml
        ${CODEGEN}/src/classes/storage.yaml
        ${CODEGEN}/src/classes/security.yaml
        ${CODEGEN}/src/classes/wasm.yaml
        ${CODEGEN}/templates/proto/types.proto.j2
        ${CODEGEN}/templates/proto/class.proto.j2
        ${CODEGEN}/templates/c/vi_types.h.j2
        ${CODEGEN}/templates/c/vi_vedam.h.j2
        ${CODEGEN}/templates/c/vii_types.h.j2
        ${CODEGEN}/templates/c/via_meta.h.j2
        ${CODEGEN}/templates/c/class_vi.h.j2
        ${CODEGEN}/templates/c/class_vii.h.j2
        ${CODEGEN}/templates/c/class_va.c.j2
        ${CODEGEN}/templates/c/class_via.c.j2
        ${CODEGEN}/templates/c/dispatch.c.j2
        ${CODEGEN}/templates/c/attr_ids.c.j2
        ${CODEGEN}/templates/c/attrwire.c.j2
        ${CODEGEN}/templates/c/class_uuid.h.j2
        ${CODEGEN}/templates/c/class_uuid.c.j2
        ${CODEGEN}/templates/cxx/codec.cc.j2
    COMMENT "Generating headers, va/via, proto, and vti into the source tree"
)

add_custom_target(vedam_codegen DEPENDS ${GEN_TREE})
