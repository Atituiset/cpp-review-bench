cmake_minimum_required(VERSION 3.10)
project(r11-partial-stage-artifact C)
add_library(r11-partial-stage-artifact STATIC src/stage.c)
target_include_directories(r11-partial-stage-artifact PUBLIC src)
