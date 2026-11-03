vedam_module(vedam_ds
    list/list.c
    stack/stack.c
    queue/queue.c
    hash/hash.c
    kv/singleton.c
    kv/index.c
    kv/hashkv.c
)
target_link_libraries(vedam_ds PUBLIC vedam_platform)
