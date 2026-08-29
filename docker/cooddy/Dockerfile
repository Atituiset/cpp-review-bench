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

# z3：cooddy 官方要求 4.8.9（系统 4.8.12 链接期 -lz3 异常），源码 build 到 /usr/local
# 产出标准 /usr/local/lib/libz3.so，链接器默认搜索路径可达
RUN cd /opt \
    && wget -q https://github.com/Z3Prover/z3/releases/download/z3-4.8.9/z3-4.8.9-x64-ubuntu-16.04.zip -O z3.zip \
    && unzip -o z3.zip -d z3_extract >/dev/null \
    && Z3DIR=$(find /opt/z3_extract -maxdepth 1 -type d -name 'z3-4.8.9-*' | head -1) \
    && cp -r "$Z3DIR/bin/libz3.so" /usr/local/lib/ \
    && cp -r "$Z3DIR/include/"* /usr/local/include/ 2>/dev/null || true \
    && cp -r "$Z3DIR/lib/"* /usr/local/lib/ 2>/dev/null || true \
    && ldconfig \
    && echo "z3 lib:" && ls -l /usr/local/lib/libz3.so

ENV Z3_HOME=/usr/local
ENV LLVM_CMAKE=/usr/lib/llvm-14/lib/cmake

RUN git clone --depth 1 https://github.com/program-analysis-team/cooddy.git /opt/cooddy \
    && cd /opt/cooddy \
    && mkdir -p build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DZ3_HOME=$Z3_HOME -DLLVM_CMAKE=$LLVM_CMAKE .. \
    && make -j "$(nproc)" \
    && echo "cooddy built:" && find /opt/cooddy/build -name cooddy -type f

WORKDIR /workspace
ENTRYPOINT ["/opt/cooddy/build/cooddy"]
