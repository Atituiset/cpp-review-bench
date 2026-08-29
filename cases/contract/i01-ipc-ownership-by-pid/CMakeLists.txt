cmake_minimum_required(VERSION 3.10)
project(i01-ipc-ownership-by-pid C)

add_library(i01-ipc-ownership-by-pid STATIC src/ipc.c)
target_include_directories(i01-ipc-ownership-by-pid PUBLIC src)
