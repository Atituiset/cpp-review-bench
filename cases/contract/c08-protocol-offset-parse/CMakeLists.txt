cmake_minimum_required(VERSION 3.16)
project(c08_protocol_offset_parse C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall)

add_library(u2u STATIC
    src/u2u_fields.c
    src/u2u_validate.c
)
