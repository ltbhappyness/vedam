add_test(NAME Codegen.Check
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/codegen/generate.py
            --root ${CMAKE_SOURCE_DIR} --check
)
