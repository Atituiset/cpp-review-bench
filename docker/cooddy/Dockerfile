# Cooddy 分析器镜像（自建，固化到 GHCR 后由 bench CI 复用）
# 依赖：clang-14 / llvm-14-dev（系统包）+ z3 4.8.9（源码 build 到 /usr/local，官方推荐版本）
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    git ca-certificates cmake gcc g++ make python3 python3-pip libz-dev libtinfo-dev wget unzip \
    clang-14 libclang-14-dev llvm-14-dev \
    && rm -rf /var/lib/apt/lists/* \
    # cooddy 要求 cmake >= 3.25，Ubuntu 22.04 自带 3.22 不足，用 pip 升到 3.25+
    && pip3 install --no-cache-dir --upgrade 'cmake>=3.25'

# z3：cooddy 官方要求 4.8.9 且需源码 build（Z3_BUILD_LIBZ3_SHARED），
# 产生 /usr/local/lib/cmake/z3/Z3Config.cmake，让 coooddy 的 find_package(Z3) 正确链接 -lz3
# （预编译二进制缺 cmake config，导致裸 -lz3 链接失败）
RUN cd /opt \
    && wget -q https://github.com/Z3Prover/z3/archive/refs/tags/z3-4.8.9.zip -O z3.zip \
    && unzip -o z3.zip >/dev/null \
    && cd z3-z3-4.8.9 \
    && mkdir -p build && cd build \
    && cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DZ3_BUILD_LIBZ3_SHARED=TRUE -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release .. \
    && make -j "$(nproc)" \
    && make install \
    && ldconfig \
    && cp -a /usr/local/lib/libz3.so* /usr/lib/x86_64-linux-gnu/ \
    && ln -sf /usr/lib/x86_64-linux-gnu/libz3.so.4.8.9.0 /usr/lib/x86_64-linux-gnu/libz3.so \
    && echo "z3 config:" && ls /usr/local/lib/cmake/z3/ 2>/dev/null && ls -l /usr/lib/x86_64-linux-gnu/libz3.so*

ENV Z3_HOME=/usr/local
ENV LLVM_CMAKE=/usr/lib/llvm-14/lib/cmake
# 强制用 gcc/g++ 作编译器与链接器驱动：gcc 的 ld 默认搜 /usr/local/lib，
# 能找到 z3 4.8.9（clang 链接器不搜 /usr/local/lib 导致 -lz3 失败）
ENV CC=gcc
ENV CXX=g++

# 关键修复：把 z3 的 lib 目录加入 LIBRARY_PATH（ld 链接 -lz3 时额外搜索路径，
# 比 CMAKE_*_LINKER_FLAGS 更可靠地让所有 target 解析到 libz3.so），并显式声明
# Z3_LIBRARIES 为绝对路径，让 cooddy 直接用绝对路径链接（不产生裸 -lz3）。
ENV LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/local/lib

RUN echo "/usr/local/lib" > /etc/ld.so.conf.d/z3.conf && ldconfig \
    && echo "=== z3 lib 布局（构建前确认 libz3.so 存在）===" \
    && ls -l /usr/local/lib/libz3.so* /usr/lib/x86_64-linux-gnu/libz3.so* 2>&1 || true

RUN git clone --depth 1 https://github.com/program-analysis-team/cooddy.git /opt/cooddy \
    # 关键修复：在 cooddy 根 CMakeLists 顶部注入 link_directories，
    # 让所有 target（含 Solver）的链接命令都带 -L 搜到 libz3.so（ld 直接调用也生效）
    && sed -i '1i link_directories(/usr/lib/x86_64-linux-gnu /usr/local/lib)' /opt/cooddy/CMakeLists.txt \
    && head -3 /opt/cooddy/CMakeLists.txt \
    && cd /opt/cooddy \
    && mkdir -p build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DZ3_HOME=$Z3_HOME -DLLVM_CMAKE=$LLVM_CMAKE \
        -DCMAKE_EXE_LINKER_FLAGS="-L/usr/local/lib -L/usr/lib/x86_64-linux-gnu" \
        -DCMAKE_SHARED_LINKER_FLAGS="-L/usr/local/lib -L/usr/lib/x86_64-linux-gnu" .. \
    && make VERBOSE=1 -j "$(nproc)" \
    && echo "cooddy built:" && find /opt/cooddy/build -name cooddy -type f

WORKDIR /workspace
ENTRYPOINT ["/opt/cooddy/build/cooddy"]
