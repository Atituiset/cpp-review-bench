#!/usr/bin/env bash
# 在装有 infer 的环境里跑：对每个 case 源文件用 infer 拦截编译，产出 infer-out/report.json
# 再把每个 case 下所有 infer-out-* 合并成归一化 findings JSON。
# 用法：run_infer.sh <cases root> <out root> [repo root]
set -u
CASES_ROOT="${1:-/workspace/cases}"
OUT_ROOT="${2:-/workspace/infer-findings}"
REPO_ROOT="${3:-$PWD}"
mkdir -p "$OUT_ROOT"

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
    infer run --compilation-database "$REPO_ROOT/build" -o "$infer_out" -- clang -c "$src" -o /dev/null 2>"$raw_dir/err.log" \
      || echo "[warn] $cid $(basename "$src") infer 失败"
  done
  # 合并该 case 所有 report.json -> 归一化 findings
  python3 "$REPO_ROOT/tools/infer_to_findings.py" "$track" "$cid" "$raw_dir" \
    --tool infer --version "$(infer --version 2>/dev/null | head -1)" \
    --out "$OUT_ROOT/$cid.json" 2>/dev/null \
    || echo "[warn] $cid 归一化失败"
  echo "[done] $cid -> $OUT_ROOT/$cid.json"
done
echo "[done] infer normalized -> $OUT_ROOT"
