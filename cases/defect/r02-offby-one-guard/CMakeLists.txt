cmake_minimum_required(VERSION 3.10)
project(r02-offby-one-guard C)
add_library(r02-offby-one-guard STATIC src/parse.c)
target_include_directories(r02-offby-one-guard PUBLIC src)
