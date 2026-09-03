#!/usr/bin/env bash
# 对每个 case 跑 clang-tidy，输出归一化 findings 到 <out_dir>/<case_id>.json
# 用法：run_clang_tidy.sh [out_dir]（默认 reports/evidence/clang-tidy）
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${1:-$ROOT/reports/evidence/clang-tidy}"
mkdir -p "$OUT"
BUILD="$ROOT/build"   # compile_commands.json 所在（cmake 生成）

CLANG_TIDY_BIN="${CLANG_TIDY_BIN:-clang-tidy-21}"
# 工具缺失属硬失败（区别于「跑通但零 findings」的合法 exit 0）
if ! command -v "$CLANG_TIDY_BIN" >/dev/null 2>&1; then
  echo "[ERROR] 找不到 clang-tidy：$CLANG_TIDY_BIN（工具缺失，CI 应变红）" >&2
  exit 127
fi
TOOL_VER="$("$CLANG_TIDY_BIN" --version 2>&1 | head -1)"
FAIL=0   # 累计工具硬失败，脚本末尾统一非零退出

for gj in "$ROOT"/cases/*/*/golden.json; do
  case_dir="$(dirname "$gj")"
  cid="$(basename "$case_dir")"
  track="$(basename "$(dirname "$case_dir")")"
  src_dir="$case_dir/src"
  [ -d "$src_dir" ] || continue
  if ! python3 "$ROOT/sa/adapters/clang_tidy_to_findings.py" "$track" "$cid" "$src_dir" \
    --clang-tidy "$CLANG_TIDY_BIN" --build-dir "$BUILD" --case-dir "$case_dir" \
    --out "$OUT/$cid.json" --tool clang-tidy --version "$TOOL_VER" \
    > "$OUT/$cid.err" 2>&1; then
    echo "[ERROR] $cid clang-tidy 执行失败：" >&2
    tail -5 "$OUT/$cid.err" >&2
    FAIL=1
    continue
  fi
done
if [ "$FAIL" -ne 0 ]; then
  echo "[ERROR] 存在 clang-tidy 硬失败（见上方 [ERROR]），以非零退出" >&2
  exit 1
fi
echo "[done] clang-tidy findings -> $OUT"
