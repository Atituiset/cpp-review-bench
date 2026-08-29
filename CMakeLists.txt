cmake_minimum_required(VERSION 3.16)
# 统一编译数据库：顶层 project() 前定为 cache 变量，确保 configure 阶段即写 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "export compile_commands.json" FORCE)

project(cpp_review_bench_top C)

# 顶层聚合构建：一键全量编译 + 统一 compile_commands.json（design §6）
# 用法： cmake -S . -B build
# 实际聚合逻辑见 cmake/AllCases.cmake
include(cmake/AllCases.cmake)
