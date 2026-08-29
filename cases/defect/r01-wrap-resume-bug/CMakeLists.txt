cmake_minimum_required(VERSION 3.10)
project(r01-wrap-resume-bug C)

add_library(r01-wrap-resume-bug STATIC src/rlc.c)
target_include_directories(r01-wrap-resume-bug PUBLIC src)
