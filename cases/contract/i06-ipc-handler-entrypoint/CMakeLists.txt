cmake_minimum_required(VERSION 3.10)
project(i06-ipc-handler-entrypoint C)

add_library(i06-ipc-handler-entrypoint STATIC src/ipc_h.c)
target_include_directories(i06-ipc-handler-entrypoint PUBLIC src)
