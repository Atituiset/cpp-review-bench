cmake_minimum_required(VERSION 3.10)
project(r12-signed-unsigned-compare C)
add_library(r12-signed-unsigned-compare STATIC src/cmp.c)
target_include_directories(r12-signed-unsigned-compare PUBLIC src)
