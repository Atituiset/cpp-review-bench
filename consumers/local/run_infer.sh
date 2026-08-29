#!/usr/bin/env bash
# 在 infer 容器内跑：对每个 case 源文件用 infer 拦截编译，产出 infer-out/report.json
# 用法（容器内）：run_infer.sh <src cases root> <out root>
set -u
CASES_ROOT="${1:-/workspace/cases}"
OUT_ROOT="${2:-/workspace/infer-findings}"
mkdir -p "$OUT_ROOT"

for gj in "$CASES_ROOT"/*/*/golden.json; do
  case_dir="$(dirname "$gj")"
  cid="$(basename "$case_dir")"
  track="$(basename "$(dirname "$case_dir")")"
  src_dir="$case_dir/src"
  [ -d "$src_dir" ] || continue
  out_dir="$OUT_ROOT/$cid"; mkdir -p "$out_dir"
  for src in "$src_dir"/*.c "$src_dir"/*.cpp; do
    [ -f "$src" ] || continue
    infer_out="$out_dir/infer-out-$(basename "$src")"
    rm -rf "$infer_out"
    infer run --compilation-database /workspace/build -o "$infer_out" -- clang -c "$src" -o /dev/null 2>"$out_dir/err.log" \
      || echo "[warn] $cid $(basename "$src") infer 失败"
  done
  echo "[done] $cid -> $out_dir"
done
echo "[done] infer raw -> $OUT_ROOT"
