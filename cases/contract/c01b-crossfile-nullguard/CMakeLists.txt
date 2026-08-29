cmake_minimum_required(VERSION 3.10)
project(c01b-crossfile-nullguard C)

add_library(c01b-crossfile-nullguard STATIC src/util.c src/caller.c)
target_include_directories(c01b-crossfile-nullguard PUBLIC src)
