cmake_minimum_required(VERSION 3.10)
project(c21-startup-global-init C)

add_library(c21-startup-global-init STATIC src/global.c)
target_include_directories(c21-startup-global-init PUBLIC src)
