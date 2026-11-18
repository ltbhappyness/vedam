if(VEDAM_NODE_CLASS STREQUAL "E")
    add_subdirectory(layering)
    add_subdirectory(codegen)
    return()
endif()

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
    if(VEDAM_ENABLE_WWLOCK)
        target_link_libraries(${name} PRIVATE wwlock)
    endif()
    target_include_directories(${name} PRIVATE ${CMAKE_BINARY_DIR})
    target_compile_options(${name} PRIVATE "-include${VEDAM_CONFIG_HEADER}")
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
add_subdirectory(via)
add_subdirectory(vta)
add_subdirectory(api)
add_subdirectory(layering)
add_subdirectory(codegen)

vedam_test(vedam_config_test
    SOURCES config/config_test.cc
    LIBS vedam
)

add_test(NAME ClassSample
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/config/class_sample.py
            --root ${CMAKE_SOURCE_DIR}
            --build ${CMAKE_BINARY_DIR}
)
set_tests_properties(ClassSample PROPERTIES RUN_SERIAL TRUE TIMEOUT 10800)
