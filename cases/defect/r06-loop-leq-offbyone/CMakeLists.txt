cmake_minimum_required(VERSION 3.10)
project(r06-loop-leq-offbyone C)
add_library(r06-loop-leq-offbyone STATIC src/fill.c)
target_include_directories(r06-loop-leq-offbyone PUBLIC src)
