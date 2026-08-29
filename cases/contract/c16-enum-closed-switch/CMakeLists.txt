cmake_minimum_required(VERSION 3.10)
project(c16-enum-closed-switch C)

add_library(c16-enum-closed-switch STATIC src/evt.c)
target_include_directories(c16-enum-closed-switch PUBLIC src)
