cmake_minimum_required(VERSION 3.10)
project(c12-intended-wrap-seq C)

add_library(c12-intended-wrap-seq STATIC src/seq.c)
target_include_directories(c12-intended-wrap-seq PUBLIC src)
