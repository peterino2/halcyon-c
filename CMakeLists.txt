cmake_minimum_required(VERSION 3.14)

project(halcyon)

add_executable( halcyon_test
    src/halcyon_test.c
    src/halc_allocators.c
    src/halc_errors.c
    src/halc_strings.c
    src/halc_tokenizer.c
    src/halc_files.c
)

set_property(TARGET halcyon_test PROPERTY C_STANDARD 99)

add_compile_options(-Wall -Werror)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release")
endif()

set_property(TARGET halcyon_test PROPERTY COMPILE_WARNING_AS_ERROR ON)

add_test(NAME halc_test COMMAND halcyon_test)
