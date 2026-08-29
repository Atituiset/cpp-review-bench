cmake_minimum_required(VERSION 3.10)
project(r05-wrong-len-var C)
add_library(r05-wrong-len-var STATIC src/copy.c)
target_include_directories(r05-wrong-len-var PUBLIC src)
