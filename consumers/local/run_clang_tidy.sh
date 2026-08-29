#!/usr/bin/env bash
# 对每个 case 跑 clang-tidy，输出归一化 findings 到 $OUT/<case_id>.json
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/reports/evidence/clang-tidy"
mkdir -p "$OUT"
BUILD="$ROOT/build"   # compile_commands.json 所在（cmake 生成）

CLANG_TIDY_BIN="${CLANG_TIDY_BIN:-clang-tidy-21}"
TOOL_VER="$("$CLANG_TIDY_BIN" --version 2>&1 | head -1)"

for gj in "$ROOT"/cases/*/*/golden.json; do
  case_dir="$(dirname "$gj")"
  cid="$(basename "$case_dir")"
  track="$(basename "$(dirname "$case_dir")")"
  src_dir="$case_dir/src"
  [ -d "$src_dir" ] || continue
  python3 "$ROOT/tools/clang_tidy_to_findings.py" "$track" "$cid" "$src_dir" \
    --clang-tidy "$CLANG_TIDY_BIN" --build-dir "$BUILD" \
    --out "$OUT/$cid.json" --tool clang-tidy --version "$TOOL_VER" \
    2>"$OUT/$cid.err" || echo "[warn] $cid clang-tidy 失败"
done
echo "[done] clang-tidy findings -> $OUT"
