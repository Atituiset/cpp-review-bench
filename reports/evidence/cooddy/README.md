# 证据：Cooddy（分支 exp/cooddy-klee，构建/运行已通，检测待调优）

Cooddy（program-analysis-team/cooddy）是华为开源的 clang 系静态分析器，自带 CWE 原生
映射（IntegerOverflowChecker=cwe-190、OutOfBoundsChecker=cwe-125/787、NullPtrDereferenceChecker
=cwe-476 等），是本 bench 最有价值的对照工具之一。

## 最终状态（2026-08-30，CI run 33293068658）
- **构建：成功**。根因突破：cooddy 的 `Solver` 目标用 `-Wl,-Bstatic -lz3` **强制静态链接
  libz3.a**，此前一直构建共享 libz3.so 导致 `ld: cannot find -lz3`。将 z3 改为
  `Z3_BUILD_LIBZ3_SHARED=FALSE` 产出 `libz3.a` 后，镜像成功构建。
- **运行：成功**。二进制 `/opt/cooddy/build/release/cooddy` 正常执行，加载全部 CWE checker
  （OutOfBounds / BufferMaxSize / NullPtrDereference / TypeSizeMismatch / UntrustedSource 等）。
- **检测：r04 实测 0 problems**。直接原因：cooddy 把 `klee_harness.c` 也纳入分析单元，该文件
  `#include <klee/klee.h>` / `recv.h` 在 cooddy 环境下找不到，单元解析失败污染上下文；且 cooddy
  需正确 `--scope` / include 路径 / 非可信源标注才能标定 `memcpy` 越界。
- **结论**：基础设施阻塞（z3 链接）已彻底解决；检测质量需 cooddy 专属**用法调优**
  （排除 harness、补 include、标 scope），属与"构建失败"性质不同的、更小的工作。

## 历史构建迭代（共 ~23 次）
1. 前 17 次（main 分支）：`-lz3` 链接失败。尝试过 Z3_LIBRARIES 绝对路径 / LIBRARY_PATH /
   link_directories 注入 / -fuse-ld=bfd / 拷贝 libz3.so 到 /usr/lib —— 均无效，
   因 cooddy 强制 `-Bstatic -lz3` 需要的是 `libz3.a` 而非 `.so`。
2. 第 18+ 次（本分支）：定位到 `-Wl,-Bstatic -lz3` → 构建静态 libz3.a → 成功。

## 复现路径
- workflow: `.github/workflows/ci.yml` 的 `cooddy-verify` job（workflow_dispatch 可手动触发）
- Dockerfile: `sa/docker/cooddy/Dockerfile`
- 成功构建 run: 33293068658（cooddy-verify conclusion=success）
- 最近一次失败 run（旧）: 33264109295（conclusion=failure，Solver 链接 -lz3）
