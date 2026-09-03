#!/usr/bin/env bash
# 对每个 case：调用 joern 容器跑 scan.sc（通用 CPG 查询：危险调用枚举，不读 golden），
# 再用 joern_to_findings.py 归一化（file 归一到 src/...、补 anchor）。
# 用法（在装有 joern 的环境，如 joern/joern 镜像）：run_joern.sh <cases root> <out root> [repo root]
set -euo pipefail
CASES_ROOT="${1:-/workspace/cases}"
OUT_ROOT="${2:-/workspace/joern-findings}"
REPO_ROOT="${3:-$PWD}"
mkdir -p "$OUT_ROOT" /tmp/joern_params

# 工具缺失属硬失败（区别于「跑通但零 findings」的合法 exit 0）
if ! command -v joern >/dev/null 2>&1; then
  echo "[ERROR] 找不到 joern（工具缺失，CI 应变红）" >&2
  exit 127
fi
FAIL=0   # 累计工具硬失败，脚本末尾统一非零退出

JOERN_VER="$(joern --version 2>/dev/null | head -1 || true)"

for gj in "$CASES_ROOT"/*/*/golden.json; do
  case_dir="$(dirname "$gj")"
  cid="$(basename "$case_dir")"
  track="$(basename "$(dirname "$case_dir")")"
  src_dir="$case_dir/src"
  [ -d "$src_dir" ] || continue

  raw="$OUT_ROOT/$cid.raw.json"; out="$OUT_ROOT/$cid.json"

  # 经环境变量传参（Joern 2.x 脚本不暴露 params map，改用 sys.env）
  if ! SRC_DIR="$src_dir" OUT_FILE="$raw" \
    joern --script "$REPO_ROOT/sa/scripts/joern/scan.sc" \
        >"/tmp/joern_params/$cid.log" 2>&1; then
    echo "[ERROR] $cid joern 执行失败（见 /tmp/joern_params/$cid.log）" >&2
    FAIL=1
  fi
  echo "--- $cid joern 日志 ---"; tail -15 "/tmp/joern_params/$cid.log" 2>/dev/null || true
  # 容忍「joern 跑通但该例无输出」：补空 findings；joern 非零已在上方计 FAIL
  [ -f "$raw" ] || echo '{"findings":[]}' > "$raw"
  if ! python3 "$REPO_ROOT/sa/adapters/joern_to_findings.py" "$track" "$cid" "$raw" \
    --case-dir "$case_dir" --version "$JOERN_VER" --out "$out" 2>"/tmp/joern_params/$cid.normalize.err"; then
    echo "[ERROR] $cid 归一化失败（见 /tmp/joern_params/$cid.normalize.err）" >&2
    FAIL=1
    continue
  fi
  echo "[done] $cid"
done
if [ "$FAIL" -ne 0 ]; then
  echo "[ERROR] 存在 joern 硬失败（见上方 [ERROR]），以非零退出" >&2
  exit 1
fi
echo "[done] joern normalized -> $OUT_ROOT"
