cmake_minimum_required(VERSION 3.10)
project(r14-buffer-len-source-untrusted C)
add_library(r14-buffer-len-source-untrusted STATIC src/ingest.c)
target_include_directories(r14-buffer-len-source-untrusted PUBLIC src)
