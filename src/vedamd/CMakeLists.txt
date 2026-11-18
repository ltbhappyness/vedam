add_executable(vedamd main.c)
target_link_libraries(vedamd PRIVATE vedam_shared)
set_target_properties(vedamd PROPERTIES
    INSTALL_RPATH "\$ORIGIN/../lib"
    BUILD_RPATH "\$ORIGIN/../lib")
