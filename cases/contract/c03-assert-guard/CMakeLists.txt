cmake_minimum_required(VERSION 3.10)
project(c03-assert-guard C)

# 统一 compdb 入口：本 case 源码加入全量构建
add_library(c03-assert-guard STATIC src/parser.c)
target_include_directories(c03-assert-guard PUBLIC src)
