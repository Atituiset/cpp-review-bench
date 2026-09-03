#!/usr/bin/env bash
# 在装有 infer 的环境里跑：对每个 case 源文件用 infer 拦截编译，产出 infer-out/report.json
# 再把每个 case 下所有 infer-out-* 合并成归一化 findings JSON。
# 用法：run_infer.sh <cases root> <out root> [repo root]
set -euo pipefail
CASES_ROOT="${1:-/workspace/cases}"
OUT_ROOT="${2:-/workspace/infer-findings}"
REPO_ROOT="${3:-$PWD}"
mkdir -p "$OUT_ROOT"

# 工具缺失属硬失败（区别于「跑通但零 findings」的合法 exit 0）
if ! command -v infer >/dev/null 2>&1; then
  echo "[ERROR] 找不到 infer（工具缺失，CI 应变红）" >&2
  exit 127
fi
FAIL=0   # 累计工具硬失败，脚本末尾统一非零退出

for gj in "$CASES_ROOT"/*/*/golden.json; do
  case_dir="$(dirname "$gj")"
  cid="$(basename "$case_dir")"
  track="$(basename "$(dirname "$case_dir")")"
  src_dir="$case_dir/src"
  [ -d "$src_dir" ] || continue
  raw_dir="$OUT_ROOT/$cid"; mkdir -p "$raw_dir"
  for src in "$src_dir"/*.c "$src_dir"/*.cpp; do
    [ -f "$src" ] || continue
    infer_out="$raw_dir/infer-out-$(basename "$src")"
    rm -rf "$infer_out"
    # 单一捕获模式：-- 后接真实编译命令（infer 文档：--compilation-database <file>
    # 与 `-- <build cmd>` 两种模式互斥；旧版混用且把 compdb 指成目录，属 bug）。
    # 逐文件捕获是刻意设计（每 case 独立 infer-out），-I 保证同目录头可解析。
    if ! infer run -o "$infer_out" -- clang -c -I"$src_dir" "$src" -o /dev/null 2>"$raw_dir/err.log"; then
      echo "[ERROR] $cid $(basename "$src") infer 执行失败（见 $raw_dir/err.log）" >&2
      FAIL=1
    fi
  done
  # 合并该 case 所有 report.json -> 归一化 findings
  if ! python3 "$REPO_ROOT/sa/adapters/infer_to_findings.py" "$track" "$cid" "$raw_dir" \
    --tool infer --version "$(infer --version 2>/dev/null | head -1)" \
    --case-dir "$case_dir" \
    --out "$OUT_ROOT/$cid.json" 2>"$raw_dir/normalize.err"; then
    echo "[ERROR] $cid 归一化失败（见 $raw_dir/normalize.err）" >&2
    FAIL=1
    continue
  fi
  echo "[done] $cid -> $OUT_ROOT/$cid.json"
done
if [ "$FAIL" -ne 0 ]; then
  echo "[ERROR] 存在 infer 硬失败（见上方 [ERROR]），以非零退出" >&2
  exit 1
fi
echo "[done] infer normalized -> $OUT_ROOT"
