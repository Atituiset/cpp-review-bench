cmake_minimum_required(VERSION 3.10)
project(r04-oob-write-stack C)
add_library(r04-oob-write-stack STATIC src/recv.c)
target_include_directories(r04-oob-write-stack PUBLIC src)
