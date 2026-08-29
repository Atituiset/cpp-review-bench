cmake_minimum_required(VERSION 3.10)
project(r07-alloc-size-wrap C)
add_library(r07-alloc-size-wrap STATIC src/alloc.c)
target_include_directories(r07-alloc-size-wrap PUBLIC src)
