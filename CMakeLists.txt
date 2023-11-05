cmake_minimum_required(VERSION 3.14)

project(halcyon)

set(CMAKE_C_FLAGS /RTC)
add_executable( halcyon_test
    src/halcyon_test.c
    src/halc_allocators.c
    src/halc_errors.c
    src/halc_strings.c
    src/halc_tokenizer.c
    src/halc_files.c
)

set_property(TARGET halcyon_test PROPERTY C_STANDARD 99)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_test(NAME halc_test COMMAND halcyon_test)
