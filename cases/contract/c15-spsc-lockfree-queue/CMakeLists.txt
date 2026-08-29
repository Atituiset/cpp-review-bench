cmake_minimum_required(VERSION 3.10)
project(c15-spsc-lockfree-queue C)

add_library(c15-spsc-lockfree-queue STATIC src/spsc.c)
target_include_directories(c15-spsc-lockfree-queue PUBLIC src)
