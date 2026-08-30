#!/usr/bin/env bash
# 本地 / CI 共用：把 CppCheck 接到 bench。
# 用法：
#   ./consumers/local/run_cppcheck.sh <output_dir>
# 产出：每个 case 一个归一化 findings JSON（tools/cppcheck_to_findings.py 输出），
#       置于 <output_dir>/<case_id>.json，可直接喂 tools/eval.py run <output_dir>
#
# 依赖：cppcheck（CI 用 apt 装，Ubuntu 源 cppcheck 2.13）。版本钉死写入 findings.version。
set -euo pipefail

OUT="${1:-/tmp/cppcheck_findings}"
CPPCHECK_BIN="${CPPCHECK_BIN:-cppcheck}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

TOOL_VER="$("$CPPCHECK_BIN" --version 2>&1 | head -1)"
mkdir -p "$OUT"

# 探测系统头目录，传给 cppcheck 避免 "include not found" 噪声并提升分析精度
INC_DIRS=()
for d in /usr/include /usr/include/x86_64-linux-gnu /usr/lib/gcc/x86_64-linux-gnu/*/include; do
  [ -d "$d" ] && INC_DIRS+=("$d")
done
INC_ARGS=()
for d in "${INC_DIRS[@]}"; do INC_ARGS+=(--include-dir "$d"); done

for gj in "$ROOT"/cases/*/*/golden.json; do
  case_dir="$(dirname "$gj")"
  cid="$(basename "$case_dir")"
  track="$(basename "$(dirname "$case_dir")")"
  src_dir="$case_dir/src"
  [ -d "$src_dir" ] || continue
  python3 "$ROOT/sa/adapters/cppcheck_to_findings.py" "$track" "$cid" "$src_dir" \
    --cppcheck "$CPPCHECK_BIN" --tool cppcheck --version "$TOOL_VER" \
    "${INC_ARGS[@]}" > "$OUT/${cid}.json"
  echo "[ok] $cid -> $OUT/${cid}.json"
done

echo "=== 评测汇总 ==="
python3 "$ROOT/tools/eval.py" run "$OUT"
