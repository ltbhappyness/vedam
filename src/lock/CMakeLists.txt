add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/wwlock
                 ${CMAKE_BINARY_DIR}/third_party/wwlock)

vedam_module(vedam_lock
    lock.c
    store.c
)
target_link_libraries(vedam_lock PRIVATE wwlock)
target_link_libraries(vedam_lock PUBLIC vedam_ds vedam_util vedam_platform)
add_dependencies(vedam_lock vedam_codegen)
