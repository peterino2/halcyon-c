cmake_minimum_required(VERSION 3.14)

project(halcyon)

add_executable( halcyon_test
    src/halcyon.c
)

set_property(TARGET halcyon_test PROPERTY C_STANDARD 99)

add_test(NAME halc_test COMMAND halcyon_test)
