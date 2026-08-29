cmake_minimum_required(VERSION 3.10)
project(r08-missing-lock-increment C)
add_library(r08-missing-lock-increment STATIC src/cnt.c)
target_include_directories(r08-missing-lock-increment PUBLIC src)
