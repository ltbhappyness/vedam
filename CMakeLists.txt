cmake_minimum_required(VERSION 3.16)
project(vedam C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(VEDAM_COVERAGE "Build with gcov instrumentation" OFF)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR CMAKE_INSTALL_PREFIX STREQUAL "${CMAKE_BINARY_DIR}/install")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Install beside the build tree" FORCE)
endif()

find_package(Protobuf REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(Threads REQUIRED)

include(cmake/VedamCodegen.cmake)
include(cmake/VedamModule.cmake)

add_subdirectory(src/platform)
add_subdirectory(src/ds)
add_subdirectory(src/util)
add_subdirectory(src/lock)
add_subdirectory(src/lib)
add_subdirectory(src/via)
add_subdirectory(src/va)
add_subdirectory(src/vta)
add_subdirectory(src/core)

set(vedam_modules
    vedam_platform
    vedam_ds
    vedam_util
    vedam_lock
    vedam_lib
    vedam_via
    vedam_va
    vedam_vta
    vedam_core
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/vi DESTINATION include)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/vti/vedam DESTINATION include/vti
    FILES_MATCHING PATTERN "*.h")

# libvedam.so holds the wwlock symbols when that path is on. libvedam.a does
# not. A static app links libvedam.a and libwwlock.a, plus libpthread, librt,
# and libprotobuf. libprotobuf stays shared, so ldd on libvedam.so lists it.
# ldd does not list wwlock.
if(VEDAM_NODE_CLASS STREQUAL "E")
    include(cmake/VedamWasm.cmake)
else()
    add_library(vedam STATIC)
    add_library(vedam_shared SHARED)
    set_target_properties(vedam_shared PROPERTIES OUTPUT_NAME vedam)
    target_link_libraries(vedam PUBLIC ${vedam_modules})
    target_link_libraries(vedam_shared PUBLIC ${vedam_modules})
    if(VEDAM_ENABLE_WWLOCK)
        target_link_libraries(vedam_shared PRIVATE wwlock)
    endif()
    add_dependencies(vedam vedam_codegen)
    add_dependencies(vedam_shared vedam_codegen)

    if(VEDAM_BUILD_VEDAMD)
        add_subdirectory(src/vedamd)
    endif()
    add_subdirectory(examples)

    set(vedam_install_targets vedam vedam_shared)
    if(VEDAM_BUILD_VEDAMD)
        list(APPEND vedam_install_targets vedamd)
    endif()
    install(TARGETS ${vedam_install_targets}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
    if(VEDAM_ENABLE_WWLOCK)
        install(TARGETS wwlock ARCHIVE DESTINATION lib)
    endif()
endif()

enable_testing()
add_subdirectory(tests)
