cmake_minimum_required(VERSION 3.16)
project(r10_odd_length_bcd C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall)

add_library(imsi_bcd STATIC src/imsi_bcd.c)
