#!/usr/bin/env bash
# 本地 / CI 共用：把 Clang Static Analyzer 接到 bench。
# 用法：
#   ./sa/runners/run_csa.sh [singletu|ctu] <output_dir>
# 产出：每个 case 一个归一化 findings JSON（sa/adapters/csa_to_findings.py 输出），
#       置于 <output_dir>/<case_id>.json，可直接喂 tools/eval.py run <output_dir>
#
# 依赖：clang / clang-extdef-mapping（ctu 模式需要）。版本钉死写入 findings.version。
set -euo pipefail

MODE="${1:-singletu}"
OUT="${2:-/tmp/csa_findings}"
CLANG_BIN="${CLANG_BIN:-clang}"
EXTDEF_BIN="${EXTDEF_BIN:-clang-extdef-mapping}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# 工具缺失属硬失败（区别于「跑通但零 findings」的合法 exit 0）
if ! command -v "$CLANG_BIN" >/dev/null 2>&1; then
  echo "[ERROR] 找不到 clang：$CLANG_BIN（工具缺失，CI 应变红）" >&2
  exit 127
fi

# 系统头：以 clang 自带 resource-dir 为主（可移植，内含 stddef/stdarg/limits 等内建头，
# 足以覆盖原硬编码 gcc include 目录的作用）；/usr/include 与 multiarch 目录为平台头补充，
# 不存在则跳过（原先硬编码 /usr/lib/gcc/x86_64-linux-gnu/13/include 是 Debian/Ubuntu
# gcc-13 专属布局，其他环境会失效，故删除，统一靠 resource-dir 兜底）
INC=""
CLANG_RES="$("$CLANG_BIN" -print-resource-dir 2>/dev/null)"
[ -n "$CLANG_RES" ] && INC="-isystem $CLANG_RES/include"
for d in /usr/include/x86_64-linux-gnu /usr/include; do
  [ -d "$d" ] && INC="$INC -isystem $d"
done

TOOL_VER="$("$CLANG_BIN" --version | head -1)"
mkdir -p "$OUT"
rm -f "$OUT/DEGRADED"   # 清掉上一次运行可能留下的退化标记
FAIL=0                  # 累计 clang --analyze 硬失败；脚本末尾据此非零退出

# CTU 退化处理：打醒目 [WARN] 并在产物目录落 DEGRADED 标记文件（CI 检测后转 ::warning::）
degrade_to_singletu() {
  local reason="$1"
  echo "[WARN] ============================================================" >&2
  echo "[WARN] CSA CTU 退化为单 TU：$reason" >&2
  echo "[WARN] 本次结果等同 singletu，已在 $OUT 落 DEGRADED 标记文件" >&2
  echo "[WARN] ============================================================" >&2
  printf '%s\n' "$reason" > "$OUT/DEGRADED"
  MODE="singletu"
}

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
    # 收集全部用例源的绝对路径（extdef-mapping 需要绝对/相对于 -p 的路径）
    SRC_LIST=()
    while IFS= read -r s; do SRC_LIST+=("$s"); done < <(python3 - "$ROOT" <<'PY'
import json, glob, os
root = os.path.abspath(os.sys.argv[1])
db = json.load(open(os.path.join(root, "build", "compile_commands.json")))
seen = set()
for e in db:
    f = e["file"]
    if not os.path.isabs(f):
        f = os.path.join(root, f)
    if f not in seen:
        seen.add(f); print(f)
PY
)
    # 方式一：clang-extdef-mapping gen <compdb>（部分版本支持）
    EXTDEF_ERR="$(mktemp)"   # 固定 /tmp 路径有并发/多用户冲突风险，改 mktemp
    if ! "$EXTDEF_BIN" gen "$COMDB" > "$CTU_DIR/externalDefMap.txt" 2>"$EXTDEF_ERR"; then
      # 方式二：clang-extdef-mapping -p <build> <sources...>（Ubuntu 包常用）
      if ! "$EXTDEF_BIN" -p "$BUILD" "${SRC_LIST[@]}" > "$CTU_DIR/externalDefMap.txt" 2>"$EXTDEF_ERR"; then
        sed 's/^/    /' "$EXTDEF_ERR" >&2
        rm -f "$EXTDEF_ERR"
        degrade_to_singletu "clang-extdef-mapping 生成 externalDefMap 失败（详见上方 stderr）"
      fi
    fi
    rm -f "$EXTDEF_ERR"
    if [ -s "$CTU_DIR/externalDefMap.txt" ]; then
      echo "[ok] externalDefMap 生成: $(wc -l < "$CTU_DIR/externalDefMap.txt") 行"
    fi
  else
    degrade_to_singletu "未找到 $EXTDEF_BIN"
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
    # clang --analyze 非零 = 编译/分析硬失败（用例保证可编译，失败即工具或环境问题），
    # 打 [ERROR] 并累计，脚本末尾统一非零退出；「跑通但 plist 零诊断」是合法结果。
    if ! "$CLANG_BIN" --analyze "${args[@]}" >"$plist_dir/${base}.log" 2>&1; then
      echo "[ERROR] $cid/$base: clang --analyze 执行失败：" >&2
      sed 's/^/    /' "$plist_dir/${base}.log" >&2
      FAIL=1
      continue
    fi
  done
  TOOL_NAME="csa-$MODE"
  [ "$MODE" = "ctu" ] && TOOL_NAME="csa-ctu"
  python3 "$ROOT/sa/adapters/csa_to_findings.py" "$track" "$cid" "$src_dir" "$plist_dir" \
    --tool "$TOOL_NAME" --version "$TOOL_VER" > "$OUT/${cid}.json"
  rm -rf "$plist_dir"
  echo "[ok] $cid -> $OUT/${cid}.json"
done

if [ "$FAIL" -ne 0 ]; then
  echo "[ERROR] 存在 clang --analyze 硬失败（见上方 [ERROR]），以非零退出" >&2
  exit 1
fi

echo "=== 评测汇总 ==="
python3 "$ROOT/tools/eval.py" run "$OUT"
