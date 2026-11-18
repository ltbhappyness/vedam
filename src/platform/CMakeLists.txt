set(vedam_platform_sources
    posix/memory.c
    posix/string.c
    posix/thread.c
    posix/sleep.c
    posix/env.c
    posix/process.c
    posix/futex.c
)
if(VEDAM_ENABLE_MQ)
    list(APPEND vedam_platform_sources posix/mq.c)
endif()

vedam_module(vedam_platform ${vedam_platform_sources})
target_link_libraries(vedam_platform PUBLIC Threads::Threads rt)
