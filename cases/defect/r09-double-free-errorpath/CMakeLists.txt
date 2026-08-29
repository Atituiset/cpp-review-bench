cmake_minimum_required(VERSION 3.10)
project(r09-double-free-errorpath C)
add_library(r09-double-free-errorpath STATIC src/ctx.c)
target_include_directories(r09-double-free-errorpath PUBLIC src)
