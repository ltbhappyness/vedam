vedam_module(vedam_platform
    posix/memory.c
    posix/string.c
    posix/thread.c
    posix/sleep.c
    posix/mq.c
    posix/env.c
    posix/process.c
    posix/futex.c
)
target_link_libraries(vedam_platform PUBLIC Threads::Threads rt)
