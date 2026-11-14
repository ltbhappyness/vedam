vedam_module(vedam_lock
    wwlock.c
    wwlock_futex.c
    lock.c
    store.c
)
target_include_directories(vedam_lock PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vedam_lock PUBLIC vedam_ds vedam_util vedam_platform)
add_dependencies(vedam_lock vedam_codegen)
