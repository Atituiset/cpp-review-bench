# 证据：Cooddy（构建镜像失败，降级待环境就绪）

Cooddy（program-analysis-team/cooddy）是华为开源的 clang 系静态分析器，自带 CWE 原生
映射（IntegerOverflowChecker=cwe-190、OutOfBoundsChecker=cwe-125/787、NullPtrDereferenceChecker
=cwe-476 等），是本 bench 最有价值的对照工具之一——它能补上 CSA/CppCheck 的 scenario=null
短板，让 eval.py 走 L1 scenario 家族精确匹配。

## 接入尝试（17 次 CI build，2026-08-29）

本仓采用「CI 里 build 一次镜像固化到 GHCR，后续复用」方案（build-cooddy-image.yml +
docker/cooddy/Dockerfile）。迭代踩坑：

1. gha 缓存 / owner 大小写 / ca-certificates / cmake 版本 / gcc+make / CMake policy 开关
   —— 均解决。
2. **z3 链接死结（未解决）**：cooddy 依赖 z3 4.8.9。系统 z3 4.8.12 与预编译二进制均导致
   `Solver` target 链接 `cannot find -lz3`。改用 z3 4.8.9 源码 build（成功产出 Z3Config.cmake
   + libz3.so.4.8.9.0），并复制到 /usr/lib/x86_64-linux-gnu/ + 建裸 libz3.so 链接——但
   cooddy 的 Solver target 链接仍 `cannot find -lz3`，且比预期更早失败（44s）。

**根因推断**：cooddy 的 `Solver` target 用裸 `-lz3` 链接，且其 CMake 配置不继承
`CMAKE_*_LINKER_FLAGS` 的 `-L`，链接器（疑似 clang driver 的 ld）默认搜索路径也未覆盖
z3 安装位置。这是 cooddy 构建系统的硬伤，非简单 Dockerfile 补丁可解。

## 降级决定

Cooddy 不阻塞本 bench 的建成线（30 例 + CSA/CppCheck + clang-tidy + Infer 等已覆盖
主流技术图谱）。待以下任一条件满足后重试：
- 源码 build LLVM14 + z3（cooddy 官方完整流程，clang 用自 build 的 sysroot 路径）
- 或找到第三方预编译 cooddy 镜像直接 pull
- 或 cooddy 上游修复 z3 链接的 CMake 配置

## 复现路径

- workflow: `.github/workflows/build-cooddy-image.yml`（workflow_dispatch 手动触发）
- Dockerfile: `docker/cooddy/Dockerfile`
- 最近一次失败 run: 33264109295（conclusion=failure，Solver 链接 -lz3）
