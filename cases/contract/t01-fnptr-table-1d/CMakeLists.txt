cmake_minimum_required(VERSION 3.10)
project(t01-fnptr-table-1d C)

add_library(t01-fnptr-table-1d STATIC src/dispatch.c)
target_include_directories(t01-fnptr-table-1d PUBLIC src)
