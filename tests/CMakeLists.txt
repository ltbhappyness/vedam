include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

function(vedam_test name)
    cmake_parse_arguments(OPT "NODE" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${name} ${OPT_SOURCES})
    target_link_libraries(${name} PRIVATE GTest::gtest_main ${OPT_LIBS})
    if(OPT_NODE)
        gtest_discover_tests(${name} PROPERTIES RESOURCE_LOCK vedam_singleton)
    else()
        gtest_discover_tests(${name})
    endif()
endfunction()

add_subdirectory(ds)
add_subdirectory(util)
add_subdirectory(platform)
add_subdirectory(lock)
add_subdirectory(vta)
add_subdirectory(api)
