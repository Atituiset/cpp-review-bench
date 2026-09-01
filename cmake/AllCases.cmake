# 顶层一键全量构建 + 统一 compile_commands.json
#
# 用法（唯一支持：从根 CMakeLists.txt include，design §6）：
#   cmake -S . -B build
#   # build/compile_commands.json 即为全仓统一编译数据库，交给 clangd/navmap/CSA 等工具
#
# 说明：
#   - 各用例自身保留独立 CMakeLists.txt（per-case project，可单独 cmake -S cases/... 构建）。
#   - 本文件聚合全部 cases/<track>/<id>/src/*.c 进一个静态库，仅用于生成 compdb，
#     不做链接（EXCLUDE_FROM_ALL），避免跨 case 符号冲突。
#   - 每个 case 的源按 <track>_<case_id>_<file> 命名目标，互不耦合。
#   - include() 不改变 CMAKE_CURRENT_SOURCE_DIR，这里它就是仓库根，基准路径按根取。

cmake_minimum_required(VERSION 3.16)
# 统一编译数据库：在 project() 前定为 cache 变量，确保 configure 阶段即写 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "export compile_commands.json" FORCE)

project(cpp_review_bench_all C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 收集全部用例源文件（cases/<track>/<id>/src/*.{c,cpp,cc,cxx}，层级固定；用 GLOB 非 RECURSE）。
# 必须包含 C++ 源，否则混合编程用例（m01/m03）的 C++ 侧不进 compile_commands.json。
file(GLOB ALL_CASE_SRCS
     ${CMAKE_CURRENT_SOURCE_DIR}/cases/*/*/src/*.c
     ${CMAKE_CURRENT_SOURCE_DIR}/cases/*/*/src/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/cases/*/*/src/*.cc
     ${CMAKE_CURRENT_SOURCE_DIR}/cases/*/*/src/*.cxx)

foreach(src ${ALL_CASE_SRCS})
    # 目标名取自相对 cases/ 的路径，避免重名
    file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR}/cases ${src})
    string(REGEX REPLACE "[/.]" "_" tgt "${rel}")
    add_library(${tgt} STATIC ${src})
    # include 目录只加该源自身所在目录（各 case 自足，不跨 case 引头文件）
    get_filename_component(src_dir ${src} DIRECTORY)
    target_include_directories(${tgt} PRIVATE ${src_dir})
    target_compile_options(${tgt} PRIVATE -Wall)
endforeach()
