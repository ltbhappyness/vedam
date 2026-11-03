cmake_minimum_required(VERSION 3.16)
project(vedam C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(VEDAM_COVERAGE "Build with gcov instrumentation" OFF)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install into the build tree" FORCE)
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
add_subdirectory(src/vta)
add_subdirectory(src/core)

add_library(vedam)
target_link_libraries(vedam PUBLIC
    vedam_platform
    vedam_ds
    vedam_util
    vedam_lock
    vedam_vta
    vedam_node
    protobuf::libprotobuf
    Threads::Threads
    rt
)
add_dependencies(vedam vedam_codegen)

add_executable(vedamd src/vedamd/main.c)
target_link_libraries(vedamd PRIVATE vedam)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/vi DESTINATION include)
install(TARGETS vedam vedamd
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

enable_testing()
add_subdirectory(tests)

add_executable(example_read_system examples/read_system.c)
target_link_libraries(example_read_system PRIVATE vedam)
