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
  harness="$REPO_ROOT/sa/harnesses/$cid/klee_harness.c"
  [ -f "$harness" ] || { echo "[skip] $cid: 无 klee_harness.c"; continue; }

  bc="$OUT_ROOT/$cid.bc"
  # 逐个编译该 case 的真实源 src/*.c 与 harness 为独立 .bc，再 llvm-link 合并，
  # 让 KLEE 符号执行真正遍历进 src 里的 sink（memcpy/越界循环），而非只停在 harness 调用点
  src_files="$(ls "$src_dir"/*.c 2>/dev/null)"
  bcs=""
  for sf in $src_files "$harness"; do
    bc_i="$OUT_ROOT/$cid.$(basename "$sf").bc"
    clang -emit-llvm -c -g -I"$src_dir" "$sf" -o "$bc_i" 2>>"$OUT_ROOT/$cid.compile.err" \
      || { echo "[warn] $cid: $(basename $sf) 编译失败"; }
    bcs="$bcs $bc_i"
  done
  llvm-link -o "$bc" $bcs 2>>"$OUT_ROOT/$cid.compile.err" \
    || { echo "[warn] $cid: llvm-link 失败"; continue; }

  # 读取 golden 期望锚点（KLEE 命中后归一到该真实源位置，便于四态评测对齐）
  gfile=$(python3 -c "import json;print(json.load(open('$gj'))['expected']['must_find'][0]['file'])" 2>/dev/null || true)
  gline=$(python3 -c "import json,glob,os;g=json.load(open('$gj'))['expected']['must_find'][0];a=g.get('anchor','');print([i+1 for i,l in enumerate(open(os.path.join('$src_dir',g['file']))) if a.strip() in l][0] if a and os.path.exists(os.path.join('$src_dir',g['file'])) else 0)" 2>/dev/null || echo 0)
  gscen=$(python3 -c "import json;print(json.load(open('$gj'))['expected']['must_find'][0].get('scenario',''))" 2>/dev/null || true)

  klee_out="$OUT_ROOT/$cid.klee"
  rm -rf "$klee_out"
  klee --output-dir="$klee_out" --max-time=60 "$bc" >"$OUT_ROOT/$cid.klee.log" 2>&1 \
    || echo "[warn] $cid klee 退出非 0（可能有错误产出）"

  python3 "$REPO_ROOT/sa/adapters/klee_to_findings.py" "$track" "$cid" "$klee_out" \
    --tool klee --version "$(klee --version 2>/dev/null | head -1)" \
    --golden-file "$gfile" --golden-line "$gline" --scenario "$gscen" \
    --out "$OUT_ROOT/$cid.json" 2>/dev/null \
    || echo "[warn] $cid 归一化失败"
  echo "[done] $cid -> $OUT_ROOT/$cid.json"
done
echo "=== 各例 KLEE findings 数 ==="
shopt -s nullglob 2>/dev/null || true
for f in "$OUT_ROOT"/*.json; do
  [ -f "$f" ] && echo "$(basename "$f"): $(python3 -c "import json;print(len(json.load(open('$f'))['findings']))" 2>/dev/null)"
done
shopt -u nullglob 2>/dev/null || true
echo "[done] klee normalized -> $OUT_ROOT"
