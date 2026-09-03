#!/usr/bin/env bash
# 对每个 case：若存在 klee_harness.c，编译为 LLVM bitcode 并跑 KLEE 符号执行，
# 再把 klee-last 的错误转成归一化 findings。
# 用法（在装有 klee + clang 的环境，如 klee/klee 镜像）：run_klee.sh <cases root> <out root> [repo root]
set -euo pipefail
CASES_ROOT="${1:-/workspace/cases}"
OUT_ROOT="${2:-/workspace/klee-findings}"
REPO_ROOT="${3:-$PWD}"
mkdir -p "$OUT_ROOT"

# 工具缺失属硬失败（区别于「跑通但零 findings」的合法 exit 0）
for bin in clang llvm-link klee; do
  if ! command -v "$bin" >/dev/null 2>&1; then
    echo "[ERROR] 找不到 $bin（工具缺失，CI 应变红）" >&2
    exit 127
  fi
done
FAIL=0   # 累计工具硬失败，脚本末尾统一非零退出
SEEN=0   # 实际跑过 harness 的用例数；为 0 说明 REPO_ROOT/harness 布局错了，属硬失败

for gj in "$CASES_ROOT"/*/*/golden.json; do
  case_dir="$(dirname "$gj")"
  cid="$(basename "$case_dir")"
  track="$(basename "$(dirname "$case_dir")")"
  src_dir="$case_dir/src"
  [ -d "$src_dir" ] || continue
  harness="$REPO_ROOT/sa/harnesses/$cid/klee_harness.c"
  [ -f "$harness" ] || { echo "[skip] $cid: 无 klee_harness.c"; continue; }
  SEEN=$((SEEN + 1))

  bc="$OUT_ROOT/$cid.bc"
  # 逐个编译该 case 的真实源 src/*.c 与 harness 为独立 .bc，再 llvm-link 合并，
  # 让 KLEE 符号执行真正遍历进 src 里的 sink（memcpy/越界循环），而非只停在 harness 调用点
  # （|| true：ls 无匹配属「该例无源文件」，非硬失败，空列表由下方循环自然跳过）
  src_files="$(ls "$src_dir"/*.c 2>/dev/null || true)"
  bcs=""
  COMPILE_OK=1
  for sf in $src_files "$harness"; do
    bc_i="$OUT_ROOT/$cid.$(basename "$sf").bc"
    # 用例与 harness 必须可编译，失败属硬失败
    if ! clang -emit-llvm -c -g -I"$src_dir" "$sf" -o "$bc_i" 2>>"$OUT_ROOT/$cid.compile.err"; then
      echo "[ERROR] $cid: $(basename "$sf") 编译为 bitcode 失败（见 $OUT_ROOT/$cid.compile.err）" >&2
      COMPILE_OK=0
      FAIL=1
    fi
    bcs="$bcs $bc_i"
  done
  [ "$COMPILE_OK" -eq 1 ] || continue
  # shellcheck disable=SC2086  # bcs 为逐文件拼接列表，需要分词
  if ! llvm-link -o "$bc" $bcs 2>>"$OUT_ROOT/$cid.compile.err"; then
    echo "[ERROR] $cid: llvm-link 失败（见 $OUT_ROOT/$cid.compile.err）" >&2
    FAIL=1
    continue
  fi

  klee_out="$OUT_ROOT/$cid.klee"
  rm -rf "$klee_out"
  # klee 非零 = 崩溃/中断（--max-time=60 正常截止是 exit 0），属硬失败，
  # 仍继续后续 case 以拿全景，但脚本末尾统一非零退出
  if ! klee --output-dir="$klee_out" --max-time=60 "$bc" >"$OUT_ROOT/$cid.klee.log" 2>&1; then
    echo "[ERROR] $cid klee 退出非 0（见 $OUT_ROOT/$cid.klee.log；klee-last 可能有部分产出）" >&2
    FAIL=1
  fi
  # klee exit 0 但连 info 文件都没落盘 = 未真正执行（输出目录参数被吞、
  # 目录结构变化等），同样属硬失败，不允许当「合法零发现」
  if [ ! -f "$klee_out/info" ]; then
    echo "[ERROR] $cid klee 未真正执行（$klee_out 下无 info 文件，见 $OUT_ROOT/$cid.klee.log）" >&2
    FAIL=1
    continue
  fi
  n_err="$(find "$klee_out" -maxdepth 1 -name '*.err' | wc -l)"

  if ! python3 "$REPO_ROOT/sa/adapters/klee_to_findings.py" "$track" "$cid" "$klee_out" \
    --tool klee --version "$(klee --version 2>/dev/null | head -1)" \
    --case-dir "$case_dir" \
    --out "$OUT_ROOT/$cid.json" 2>"$OUT_ROOT/$cid.normalize.err"; then
    echo "[ERROR] $cid 归一化失败（见 $OUT_ROOT/$cid.normalize.err）" >&2
    FAIL=1
    continue
  fi
  # adapter 的降级 warning（如 anchor 合成失败省略 finding）必须浮出水面：
  # 历史上「klee 检出 N 个错误但 findings 全被静默省略」就是这样被当成合法零发现的
  if [ -s "$OUT_ROOT/$cid.normalize.err" ]; then
    sed 's/^/[warn] /' "$OUT_ROOT/$cid.normalize.err" >&2
  fi
  n_findings="$(python3 -c "import json;print(len(json.load(open('$OUT_ROOT/$cid.json'))['findings']))" 2>/dev/null || echo 0)"
  if [ "$n_err" -gt 0 ] && [ "$n_findings" -eq 0 ]; then
    echo "[warn] $cid: klee 产出 $n_err 个 .err 但归一化后 findings=0（检出被全部省略，查 $OUT_ROOT/$cid.normalize.err）" >&2
  fi
  echo "[done] $cid -> $OUT_ROOT/$cid.json（err=$n_err findings=$n_findings）"
done
echo "=== 各例 KLEE findings 数 ==="
shopt -s nullglob
for f in "$OUT_ROOT"/*.json; do
  [ -f "$f" ] && echo "$(basename "$f"): $(python3 -c "import json;print(len(json.load(open('$f'))['findings']))" 2>/dev/null || echo '?')"
done
shopt -u nullglob
if [ "$SEEN" -eq 0 ]; then
  echo "[ERROR] 未跑任何 harness（全部 [skip]）：REPO_ROOT=$REPO_ROOT 下 sa/harnesses 布局错误或路径指错，不允许当合法零发现" >&2
  exit 1
fi
if [ "$FAIL" -ne 0 ]; then
  echo "[ERROR] 存在 klee 硬失败（见上方 [ERROR]），以非零退出" >&2
  exit 1
fi
echo "[done] klee normalized -> $OUT_ROOT"
