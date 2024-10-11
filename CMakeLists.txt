cmake_minimum_required(VERSION 3.14)

project(halcyon)

add_executable( halcyon_test
    src/halcyon_test.cpp
    src/halc_allocators.c
    src/halc_errors.c
    src/halc_strings.c
    src/halc_tokenizer.c
    src/halc_files.c
    src/halc_parser.c
)

set_property(TARGET halcyon_test PROPERTY C_STANDARD 99)

add_compile_options(-Wall -Werror)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)
add_compile_options(-g)
target_include_directories(halcyon_test PUBLIC "include/")

if(CMAKE_BUILD_TYPE MATCHES "Debug")
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release")
endif()

set_property(TARGET halcyon_test PROPERTY COMPILE_WARNING_AS_ERROR ON)

add_test(NAME halc_test COMMAND halcyon_test)
