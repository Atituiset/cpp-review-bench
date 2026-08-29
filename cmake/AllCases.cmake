# 顶层一键全量构建 + 统一 compile_commands.json
#
# 用法（design §6）：
#   cmake -S cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
#   # build/compile_commands.json 即为全仓统一编译数据库，交给 clangd/navmap/CSA 等工具
#
# 说明：
#   - 各用例自身保留独立 CMakeLists.txt（per-case project，可单独 cmake -S cases/... 构建）。
#   - 本文件聚合全部 cases/<track>/<id>/src/*.c 进一个静态库，仅用于生成 compdb，
#     不做链接（EXCLUDE_FROM_ALL），避免跨 case 符号冲突。
#   - 每个 case 的源按 <case_id> 命名目标，互不耦合。

cmake_minimum_required(VERSION 3.16)
# 统一编译数据库：在 project() 前定为 cache 变量，确保 configure 阶段即写 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "export compile_commands.json" FORCE)

project(cpp_review_bench_all C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 收集全部用例源文件（cases/<track>/<id>/src/*.c，层级固定；用 GLOB 非 RECURSE）
file(GLOB ALL_CASE_SRCS
     ${CMAKE_CURRENT_SOURCE_DIR}/cases/*/*/src/*.c)

foreach(src ${ALL_CASE_SRCS})
    # 目标名取自相对 cases/ 的路径，避免重名
    file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR}/../cases ${src})
    string(REGEX REPLACE "[/.]" "_" tgt "${rel}")
    add_library(${tgt} STATIC ${src})
    target_include_directories(${tgt} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../cases/contract/*/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../cases/defect/*/src)
    target_compile_options(${tgt} PRIVATE -Wall)
endforeach()
