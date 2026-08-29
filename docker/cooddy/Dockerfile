# Cooddy 分析器镜像（自建，固化到 GHCR 后由 bench CI 复用）
# 依赖：clang-14 / llvm-14-dev / z3（系统包，避免源码 build LLVM 14）
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    git ca-certificates cmake gcc g++ make python3 python3-pip libz-dev libtinfo-dev wget unzip \
    clang-14 libclang-14-dev llvm-14-dev \
    z3 libz3-dev \
    && rm -rf /var/lib/apt/lists/* \
    # cooddy 要求 cmake >= 3.25，Ubuntu 22.04 自带 3.22 不足，用 pip 升到 3.25+
    && pip3 install --no-cache-dir --upgrade 'cmake>=3.25'

# cooddy 的 CMake 需要 Z3_HOME / LLVM_CMAKE 指向系统安装
ENV Z3_HOME=/usr
ENV LLVM_CMAKE=/usr/lib/llvm-14/lib/cmake

RUN git clone --depth 1 https://github.com/program-analysis-team/cooddy.git /opt/cooddy \
    && cd /opt/cooddy \
    && mkdir -p build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DZ3_HOME=$Z3_HOME -DLLVM_CMAKE=$LLVM_CMAKE .. \
    && make -j "$(nproc)" \
    && echo "cooddy built:" && find /opt/cooddy/build -name cooddy -type f

WORKDIR /workspace
ENTRYPOINT ["/opt/cooddy/build/cooddy"]
