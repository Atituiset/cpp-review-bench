cmake_minimum_required(VERSION 3.10)
project(m03-c-guard-cpp-deref CXX)

add_library(m03-c-guard-cpp-deref STATIC src/engine.cpp src/centry.c src/bind.cpp)
target_include_directories(m03-c-guard-cpp-deref PUBLIC src)
