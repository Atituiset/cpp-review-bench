cmake_minimum_required(VERSION 3.10)
project(m01-extern-c-entrypoint CXX)

add_library(m01-extern-c-entrypoint STATIC src/entry.cpp src/caller.c)
target_include_directories(m01-extern-c-entrypoint PUBLIC src)
