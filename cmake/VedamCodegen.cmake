# Regenerate committed sources when the model, the generator, or the templates change.

set(CODEGEN "${CMAKE_SOURCE_DIR}/tools/codegen")
set(MODEL "${CMAKE_SOURCE_DIR}/model")
set(VEDAM_CLASSES system identity storage security wasm)

set(GEN_TREE
    ${CMAKE_SOURCE_DIR}/include/vi/types.h
    ${CMAKE_SOURCE_DIR}/include/vi/vedam.h
    ${CMAKE_SOURCE_DIR}/include/vii/types.h
    ${CMAKE_SOURCE_DIR}/include/vii/meta.h
    ${CMAKE_SOURCE_DIR}/src/via/classes.c
    ${CMAKE_SOURCE_DIR}/include/vti/class_uuid.h
    ${CMAKE_SOURCE_DIR}/src/vta/class_uuid.c
    ${CMAKE_SOURCE_DIR}/src/vta/struct_codec.cc
    ${CMAKE_SOURCE_DIR}/proto/vedam/v1/types.proto
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/v1/types.pb.cc
    ${CMAKE_SOURCE_DIR}/include/vti/vedam/v1/types.pb.h
)
set(GEN_INPUTS
    ${MODEL}/types.yaml
    ${MODEL}/schema/types.schema.yaml
    ${MODEL}/schema/class.schema.yaml
)
foreach(cls IN LISTS VEDAM_CLASSES)
    list(APPEND GEN_TREE
        ${CMAKE_SOURCE_DIR}/include/vi/class/${cls}.h
        ${CMAKE_SOURCE_DIR}/include/vii/class/${cls}.h
        ${CMAKE_SOURCE_DIR}/src/va/${cls}.c
        ${CMAKE_SOURCE_DIR}/proto/vedam/${cls}/v1/${cls}.proto
        ${CMAKE_SOURCE_DIR}/include/vti/vedam/${cls}/v1/${cls}.pb.cc
        ${CMAKE_SOURCE_DIR}/include/vti/vedam/${cls}/v1/${cls}.pb.h
    )
    list(APPEND GEN_INPUTS ${MODEL}/classes/${cls}.yaml)
endforeach()

add_custom_command(
    OUTPUT ${GEN_TREE}
    COMMAND ${Python3_EXECUTABLE} ${CODEGEN}/generate.py --root ${CMAKE_SOURCE_DIR}
    DEPENDS
        ${GEN_INPUTS}
        ${CODEGEN}/generate.py
        ${CODEGEN}/infra/context.py
        ${CODEGEN}/infra/copyright.py
        ${CODEGEN}/infra/emit.py
        ${CODEGEN}/infra/model.py
        ${CODEGEN}/infra/render.py
        ${CODEGEN}/infra/schema.py
        ${CODEGEN}/templates/proto/types.proto.j2
        ${CODEGEN}/templates/proto/class.proto.j2
        ${CODEGEN}/templates/c/vi_types.h.j2
        ${CODEGEN}/templates/c/vi_vedam.h.j2
        ${CODEGEN}/templates/c/vii_types.h.j2
        ${CODEGEN}/templates/c/vii_meta.h.j2
        ${CODEGEN}/templates/c/classes.c.j2
        ${CODEGEN}/templates/c/class_vi.h.j2
        ${CODEGEN}/templates/c/class_vii.h.j2
        ${CODEGEN}/templates/c/class_va.c.j2
        ${CODEGEN}/templates/c/class_uuid.h.j2
        ${CODEGEN}/templates/c/class_uuid.c.j2
        ${CODEGEN}/templates/cxx/struct_codec.cc.j2
    COMMENT "Generating headers, va, via classes, proto, and vti into the source tree"
)

add_custom_target(vedam_codegen DEPENDS ${GEN_TREE})
