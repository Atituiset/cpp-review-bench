cmake_minimum_required(VERSION 3.10)
project(r03-public-entry-bypass C)
add_library(r03-public-entry-bypass STATIC src/entry.c)
target_include_directories(r03-public-entry-bypass PUBLIC src)
