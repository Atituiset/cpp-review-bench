#!/usr/bin/env bash
# 对每个 case：从 golden.json 取出 must_find[0] 的 anchor/function/scenario，
# 写入临时文件，调用 joern 容器跑 scan.sc（CPG 图查询定位锚点 + 危险调用枚举），
# 再用 joern_to_findings.py 归一化。
# 用法（在装有 joern 的环境，如 joern/joern 镜像）：run_joern.sh <cases root> <out root> [repo root]
set -u
CASES_ROOT="${1:-/workspace/cases}"
OUT_ROOT="${2:-/workspace/joern-findings}"
REPO_ROOT="${3:-$PWD}"
mkdir -p "$OUT_ROOT" /tmp/joern_params

for gj in "$CASES_ROOT"/*/*/golden.json; do
  case_dir="$(dirname "$gj")"
  cid="$(basename "$case_dir")"
  track="$(basename "$(dirname "$case_dir")")"
  src_dir="$case_dir/src"
  [ -d "$src_dir" ] || continue

  anchor=$(python3 -c "import json;d=json.load(open('$gj'));print(d['expected']['must_find'][0]['anchor'])" 2>/dev/null || true)
  function=$(python3 -c "import json;d=json.load(open('$gj'));print(d['expected']['must_find'][0]['function'])" 2>/dev/null || true)
  scenario=$(python3 -c "import json;d=json.load(open('$gj'));print(d['expected']['must_find'][0].get('scenario',''))" 2>/dev/null || true)

  af="/tmp/joern_params/$cid.anchor"; echo "$anchor" > "$af"
  ff="/tmp/joern_params/$cid.func";   echo "$function" > "$ff"
  raw="$OUT_ROOT/$cid.raw.json"; out="$OUT_ROOT/$cid.json"

  joern --script "$REPO_ROOT/joern-scripts/scan.sc" \
        --params "srcDir=$src_dir,anchorFile=$af,functionFile=$ff,scenario=$scenario,outFile=$raw" \
        >/dev/null 2>&1 || echo "[warn] $cid joern 失败"
  [ -f "$raw" ] || echo '{"findings":[]}' > "$raw"
  python3 "$REPO_ROOT/tools/joern_to_findings.py" "$track" "$cid" "$raw" --out "$out" 2>/dev/null \
    || echo "[warn] $cid 归一化失败"
  echo "[done] $cid"
done
echo "[done] joern normalized -> $OUT_ROOT"
