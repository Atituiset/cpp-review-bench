#!/usr/bin/env bash
# 对每个 case：若存在 klee_harness.c，编译为 LLVM bitcode 并跑 KLEE 符号执行，
# 再把 klee-last 的错误转成归一化 findings。
# 用法（在装有 klee + clang 的环境，如 klee/klee 镜像）：run_klee.sh <cases root> <out root> [repo root]
set -u
CASES_ROOT="${1:-/workspace/cases}"
OUT_ROOT="${2:-/workspace/klee-findings}"
REPO_ROOT="${3:-$PWD}"
mkdir -p "$OUT_ROOT"

for gj in "$CASES_ROOT"/*/*/golden.json; do
  case_dir="$(dirname "$gj")"
  cid="$(basename "$case_dir")"
  track="$(basename "$(dirname "$case_dir")")"
  src_dir="$case_dir/src"
  [ -d "$src_dir" ] || continue
  harness="$src_dir/klee_harness.c"
  [ -f "$harness" ] || { echo "[skip] $cid: 无 klee_harness.c"; continue; }

  bc="$OUT_ROOT/$cid.bc"
  # 编译 harness + 该 case 的所有源为 bitcode
  clang -emit-llvm -c -g -I"$src_dir" "$harness" -o "$bc" 2>"$OUT_ROOT/$cid.compile.err" \
    || { echo "[warn] $cid: bitcode 编译失败"; continue; }

  klee_out="$OUT_ROOT/$cid.klee"
  rm -rf "$klee_out"
  klee --output-dir="$klee_out" --max-time=60 "$bc" >"$OUT_ROOT/$cid.klee.log" 2>&1 \
    || echo "[warn] $cid klee 退出非 0（可能有错误产出）"

  python3 "$REPO_ROOT/tools/klee_to_findings.py" "$track" "$cid" "$klee_out" \
    --tool klee --version "$(klee --version 2>/dev/null | head -1)" \
    --out "$OUT_ROOT/$cid.json" 2>/dev/null \
    || echo "[warn] $cid 归一化失败"
  echo "[done] $cid -> $OUT_ROOT/$cid.json"
done
echo "[done] klee normalized -> $OUT_ROOT"
