#!/usr/bin/env bash
# 对每个 case：调用 joern 容器跑 scan.sc（通用 CPG 查询：危险调用枚举，不读 golden），
# 再用 joern_to_findings.py 归一化（file 归一到 src/...、补 anchor）。
# 用法（在装有 joern 的环境，如 joern/joern 镜像）：run_joern.sh <cases root> <out root> [repo root]
set -u
CASES_ROOT="${1:-/workspace/cases}"
OUT_ROOT="${2:-/workspace/joern-findings}"
REPO_ROOT="${3:-$PWD}"
mkdir -p "$OUT_ROOT" /tmp/joern_params

JOERN_VER="$(joern --version 2>/dev/null | head -1)"

for gj in "$CASES_ROOT"/*/*/golden.json; do
  case_dir="$(dirname "$gj")"
  cid="$(basename "$case_dir")"
  track="$(basename "$(dirname "$case_dir")")"
  src_dir="$case_dir/src"
  [ -d "$src_dir" ] || continue

  raw="$OUT_ROOT/$cid.raw.json"; out="$OUT_ROOT/$cid.json"

  # 经环境变量传参（Joern 2.x 脚本不暴露 params map，改用 sys.env）
  SRC_DIR="$src_dir" OUT_FILE="$raw" \
  joern --script "$REPO_ROOT/sa/scripts/joern/scan.sc" \
        >"/tmp/joern_params/$cid.log" 2>&1 || echo "[warn] $cid joern 失败（见 /tmp/joern_params/$cid.log）"
  echo "--- $cid joern 日志 ---"; tail -15 "/tmp/joern_params/$cid.log" 2>/dev/null
  [ -f "$raw" ] || echo '{"findings":[]}' > "$raw"
  python3 "$REPO_ROOT/sa/adapters/joern_to_findings.py" "$track" "$cid" "$raw" \
    --case-dir "$case_dir" --version "$JOERN_VER" --out "$out" 2>/dev/null \
    || echo "[warn] $cid 归一化失败"
  echo "[done] $cid"
done
echo "[done] joern normalized -> $OUT_ROOT"
