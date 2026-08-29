#!/usr/bin/env bash
# 本地 / CI 共用：把 Clang Static Analyzer 接到 bench。
# 用法：
#   ./consumers/local/run_csa.sh [singletu|ctu] <output_dir>
# 产出：每个 case 一个归一化 findings JSON（tools/csa_to_findings.py 输出），
#       置于 <output_dir>/<case_id>.json，可直接喂 tools/eval.py run <output_dir>
#
# 依赖：clang / clang-extdef-mapping（ctu 模式需要）。版本钉死写入 findings.version。
set -euo pipefail

MODE="${1:-singletu}"
OUT="${2:-/tmp/csa_findings}"
CLANG_BIN="${CLANG_BIN:-clang}"
EXTDEF_BIN="${EXTDEF_BIN:-clang-extdef-mapping}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

INC="-isystem /usr/lib/gcc/x86_64-linux-gnu/13/include -isystem /usr/include/x86_64-linux-gnu -isystem /usr/include"
# 若非 Debian/Ubuntu，回退到 clang 自带资源头
CLANG_VER="$("$CLANG_BIN" --version | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)"
CLANG_RES="$("$CLANG_BIN" -print-resource-dir 2>/dev/null)"
[ -n "$CLANG_RES" ] && INC="$INC -isystem $CLANG_RES/include"

TOOL_VER="$("$CLANG_BIN" --version | head -1)"
mkdir -p "$OUT"

# 统一编译数据库（CTU 需要它生成 externalDefMap）
BUILD="$ROOT/build"
if [ ! -f "$BUILD/compile_commands.json" ]; then
  cmake -S "$ROOT" -B "$BUILD" -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON >/dev/null 2>&1
fi
COMDB="$BUILD/compile_commands.json"

# CTU 准备：生成跨 TU 外部定义映射
CTU_DIR=""
if [ "$MODE" = "ctu" ]; then
  CTU_DIR="$OUT/ctu"
  mkdir -p "$CTU_DIR"
  if command -v "$EXTDEF_BIN" >/dev/null 2>&1; then
    "$EXTDEF_BIN" gen "$COMDB" > "$CTU_DIR/externalDefMap.txt" 2>/dev/null || \
      echo "[warn] clang-extdef-mapping gen 失败，CTU 退化为单 TU" >&2
  else
    echo "[warn] 未找到 $EXTDEF_BIN，CTU 退化为单 TU" >&2
    MODE="singletu"
  fi
fi

for gj in "$ROOT"/cases/*/*/golden.json; do
  case_dir="$(dirname "$gj")"
  cid="$(basename "$case_dir")"
  track="$(basename "$(dirname "$case_dir")")"
  src_dir="$case_dir/src"
  [ -d "$src_dir" ] || continue
  plist_dir="$(mktemp -d)"
  for c in "$src_dir"/*.c; do
    [ -f "$c" ] || continue
    base="$(basename "$c" .c)"
    args=(-Xanalyzer -analyzer-output=plist -o "$plist_dir/${base}.plist" -std=c11 $INC "$c")
    if [ "$MODE" = "ctu" ] && [ -n "$CTU_DIR" ] && [ -s "$CTU_DIR/externalDefMap.txt" ]; then
      args=(-Xanalyzer -analyzer-config -Xanalyzer ctu-dir="$CTU_DIR" \
            -Xanalyzer -analyzer-config -Xanalyzer experimental-enable-naive-ctu=true \
            "${args[@]}")
    fi
    "$CLANG_BIN" --analyze "${args[@]}" >/dev/null 2>&1 || true
  done
  TOOL_NAME="csa-$MODE"
  [ "$MODE" = "ctu" ] && TOOL_NAME="csa-ctu"
  python3 "$ROOT/tools/csa_to_findings.py" "$track" "$cid" "$src_dir" "$plist_dir" \
    --tool "$TOOL_NAME" --version "$TOOL_VER" > "$OUT/${cid}.json"
  rm -rf "$plist_dir"
  echo "[ok] $cid -> $OUT/${cid}.json"
done

echo "=== 评测汇总 ==="
python3 "$ROOT/tools/eval.py" run "$OUT"
