# Cooddy 分析器镜像（自建，固化到 GHCR 后由 bench CI 复用）
# 依赖：clang-14 / llvm-14-dev / z3（系统包，避免源码 build LLVM 14）
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    git ca-certificates cmake g++ python3 libz-dev libtinfo-dev wget unzip \
    clang-14 libclang-14-dev llvm-14-dev \
    z3 libz3-dev \
    && rm -rf /var/lib/apt/lists/*

# cooddy 的 CMake 需要 Z3_HOME / LLVM_CMAKE 指向系统安装
ENV Z3_HOME=/usr
ENV LLVM_CMAKE=/usr/lib/llvm-14/lib/cmake

RUN git clone --depth 1 https://github.com/program-analysis-team/cooddy.git /opt/cooddy \
    && cd /opt/cooddy \
    && mkdir -p build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release -DZ3_HOME=$Z3_HOME -DLLVM_CMAKE=$LLVM_CMAKE .. \
    && make -j "$(nproc)" \
    && echo "cooddy built:" && find /opt/cooddy/build -name cooddy -type f

WORKDIR /workspace
ENTRYPOINT ["/opt/cooddy/build/cooddy"]
