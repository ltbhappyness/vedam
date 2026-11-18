if(VEDAM_ENABLE_WWLOCK)
    add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/wwlock
                     ${CMAKE_BINARY_DIR}/third_party/wwlock)
    vedam_module(vedam_lock
        lock.c
        store.c
    )
    target_link_libraries(vedam_lock PUBLIC wwlock)
else()
    vedam_module(vedam_lock
        nolock.c
    )
endif()
target_link_libraries(vedam_lock PUBLIC vedam_ds vedam_util vedam_platform)
add_dependencies(vedam_lock vedam_codegen)
