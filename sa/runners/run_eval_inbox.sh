#!/usr/bin/env bash
# run_eval_inbox.sh —— 对 harvest/inbox/draft 候选跑轻量 SA 评测，产出「该真实 bug 片段能否被标出」报表。
#
# 设计：inbox 候选是外部开源仓的函数片段，无法在 bench 完整构建环境编译（缺外部仓头文件/依赖），
# 故用单文件 SA（clang --analyze / cppcheck）评估「片段自身能否被 SA 标出缺陷信号」——
# 这是 M2 评测环的务实形态；确认进 cases/ 后再走完整 9 工具（仓根 ci.yml）。
#
# 输入：
#   $1 = 仓根（REPO_ROOT）
#   $2 = inbox 根（harvest/inbox）
#   $3 = 报表输出目录
# 输出：
#   $3/eval_inbox_report.json  每候选的 {case_id, scenario, csa_hit, cppcheck_hit, verdict}
#   $3/eval_inbox_report.md    人审用摘要（附 PR body）
set -u

REPO_ROOT="${1:?用法: $0 <repo_root> <inbox_root> <out_dir>}"
INBOX="${2:?}"
OUT="${3:?}"
mkdir -p "$OUT"

csa_hit() {  # $1=src 文件；stdout: 命中的 checker 名列表（去重）
  local f="$1"
  local plist; plist="$(mktemp --suffix=.plist)"
  clang --analyze -Xanalyzer -analyzer-output=plist "$f" -o "$plist" >/dev/null 2>&1 || true
  if [ -f "$plist" ]; then
    # 从 plist 提 description（含 checker 族）
    grep -aoE 'core\.[A-Za-z]+|cplusplus\.[A-Za-z]+|alpha\.[A-Za-z.]+' "$plist" 2>/dev/null | sort -u
  fi
  rm -f "$plist"
}
cppcheck_hit() {
  local f="$1"
  cppcheck --enable=warning,style,performance,portability --xml --xml-version=2 "$f" 2>/dev/null \
    | grep -aoE '<error id="[^"]+"' | sed -E 's/<error id="//; s/"//' | sort -u
}

verdict() {
  # $1=csa_hits(空格分隔) $2=cppcheck_hits $3=scenario
  local csa="$1" cpp="$2" scen="$3"
  if echo "$csa" | grep -qi "${scen#cwe-}"; then echo "detected-by-csa"; return; fi
  if echo "$cpp" | grep -qi "${scen#cwe-}"; then echo "detected-by-cppcheck"; return; fi
  if [ -n "$csa" ] || [ -n "$cpp" ]; then echo "sa-flagged-other"; return; fi
  echo "sa-silent"
}

echo "=== 评测 inbox 候选（轻量单文件 SA）==="
find "$INBOX/draft" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | while read -r casedir; do
  cid="$(basename "$casedir")"
  # 读 golden 草稿的 scenario
  scen="$(python3 -c "import json,sys; g=json.load(open('$casedir/golden.json')); print(g['must_find'][0]['scenario'])" 2>/dev/null || echo unknown)"
  srcf="$(find "$casedir/src" -name '*.c' -o -name '*.cpp' -o -name '*.cc' 2>/dev/null | head -1)"
  csa="" cpp=""
  if [ -n "$srcf" ]; then
    csa="$(csa_hit "$srcf" | tr '\n' ' ')"
    cpp="$(cppcheck_hit "$srcf" | tr '\n' ' ')"
  fi
  v="$(verdict "$csa" "$cpp" "$scen")"
  echo "  $cid: scenario=$scen -> $v"
  # 累积 json
  python3 - "$cid" "$scen" "$v" "$csa" "$cpp" <<'PY' >> "$OUT/_rows.jsonl"
import json,sys
cid,scen,v,csa,cpp=sys.argv[1:6]
row={"case_id":cid,"scenario":scen,"csa_hits":csa.strip().split(),"cppcheck_hits":cpp.strip().split(),"verdict":v}
print(json.dumps(row,ensure_ascii=False))
PY
done

# 汇总 json + md
python3 - <<'PY'
import json,collections
rows=[json.loads(l) for l in open("/tmp/_rows.jsonl")] if False else []
# 上面 while 子 shell 的 _rows.jsonl 在 $OUT
import os
p="$OUT/_rows.jsonl"
rows=[]
if os.path.exists(p):
    rows=[json.loads(l) for l in open(p)]
vc=collections.Counter(r["verdict"] for r in rows)
summary={"total":len(rows),"verdicts":dict(vc)}
with open("$OUT/eval_inbox_report.json","w") as f:
    json.dump({"summary":summary,"cases":rows},f,indent=2,ensure_ascii=False)
with open("$OUT/eval_inbox_report.md","w") as f:
    f.write(f"# inbox 候选 SA 评测报表（{len(rows)} 条）\n\n")
    f.write("| 候选 | scenario | CSA | CppCheck | 结论 |\n|---|---|---|---|---|\n")
    for r in rows:
        f.write(f"| {r['case_id']} | {r['scenario']} | {';'.join(r['csa_hits']) or '-'} | {';'.join(r['cppcheck_hits']) or '-'} | {r['verdict']} |\n")
    f.write(f"\n**汇总**：{json.dumps(summary['verdicts'],ensure_ascii=False)}\n")
    f.write("\n> 说明：轻量单文件 SA（clang --analyze + cppcheck）仅评估片段自身能否被标出；")
    f.write("确认进 cases/ 后走完整 9 工具（仓根 ci.yml）。\n")
print("报表 →", "$OUT/eval_inbox_report.md")
PY
rm -f "$OUT/_rows.jsonl"
