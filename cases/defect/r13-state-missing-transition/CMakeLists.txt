cmake_minimum_required(VERSION 3.10)
project(r13-state-missing-transition C)
add_library(r13-state-missing-transition STATIC src/fsm.c)
target_include_directories(r13-state-missing-transition PUBLIC src)
