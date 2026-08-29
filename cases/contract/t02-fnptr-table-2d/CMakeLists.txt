cmake_minimum_required(VERSION 3.10)
project(t02-fnptr-table-2d C)

add_library(t02-fnptr-table-2d STATIC src/dispatch2.c)
target_include_directories(t02-fnptr-table-2d PUBLIC src)
