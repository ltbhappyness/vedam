# One object library. The root vedam target links these together.

function(vedam_module name)
    add_library(${name} OBJECT ${ARGN})
    target_include_directories(${name} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_compile_features(${name} PUBLIC c_std_11 cxx_std_17)
    if(VEDAM_COVERAGE)
        target_compile_options(${name} PRIVATE --coverage)
        target_link_options(${name} INTERFACE --coverage)
    endif()
    target_compile_options(${name} PRIVATE "-include${VEDAM_CONFIG_HEADER}")
endfunction()
