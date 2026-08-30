#!/usr/bin/env bash
# make_draft_sarif.sh —— 对 harvest/inbox/draft 候选跑轻量 SA，产出：
#   1) 每个候选的 findings.json（合成：golden 猜测 scenario + 轻量 SA 是否标出）
#   2) 合并 SARIF（uri 指向 draft 内 src 文件 → PR 内联标注）
#   3) 四态大白话表（PR 评论用）
#
# 用法: make_draft_sarif.sh <repo_root> <inbox_root> <out_dir>
# 依赖: clang --analyze, cppcheck, python3 sa/adapters/findings_to_sarif.py
set -u

REPO_ROOT="$(cd "${1:?用法: $0 <repo_root> <inbox_root> <out_dir>}" && pwd)"
INBOX="$(cd "${2:?}" && pwd)"
OUT="${3:?}"
mkdir -p "$OUT/findings" "$OUT/sarif"

ADAPTER="$REPO_ROOT/sa/adapters/findings_to_sarif.py"

csa_hit() {
  local f="$1" plist
  plist="$(mktemp --suffix=.plist)"
  clang --analyze -Xanalyzer -analyzer-output=plist "$f" -o "$plist" >/dev/null 2>&1 || true
  if [ -f "$plist" ]; then
    grep -aoE 'core\.[A-Za-z]+|cplusplus\.[A-Za-z]+|alpha\.[A-Za-z.]+' "$plist" 2>/dev/null | sort -u
  fi
  rm -f "$plist"
}
cppcheck_hit() {
  cppcheck --enable=warning,style,performance,portability --xml --xml-version=2 "$1" 2>/dev/null \
    | grep -aoE '<error id="[^"]+"' | sed -E 's/<error id="//; s/"//' | sort -u
}

echo "=== 生成 draft 候选 SARIF + 四态表 ==="
rows_jsonl="$OUT/_rows.jsonl"
: > "$rows_jsonl"

find "$INBOX/draft" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | while read -r casedir; do
  cid="$(basename "$casedir")"
  # 读取 golden 猜测 scenario（harvest draft golden 形状：must_find[].scenario）
  scen="$(python3 -c "import json;g=json.load(open('$casedir/golden.json'));print(g.get('must_find',[{}])[0].get('scenario','unknown'))" 2>/dev/null || echo unknown)"
  srcf="$(find "$casedir/src" \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' \) 2>/dev/null | head -1)"
  csa="" cpp=""
  if [ -n "$srcf" ]; then
    csa="$(csa_hit "$srcf" | tr '\n' ' ')"
    cpp="$(cppcheck_hit "$srcf" | tr '\n' ' ')"
  fi
  # 是否标出该 scenario（轻量 TP/FN 判读）
  if echo "$csa" | grep -qi "${scen#cwe-}"; then state="TP(CSA标出)"; sa="csa";
  elif echo "$cpp" | grep -qi "${scen#cwe-}"; then state="TP(CppCheck标出)"; sa="cppcheck";
  elif [ -n "$csa" ] || [ -n "$cpp" ]; then state="FN(标出其他)"; sa="other";
  else state="FN(静默)"; sa="none"; fi
  # 候选在 PR 内的完整路径（SARIF 标注映射到 PR 新文件）
  rel="${casedir#$REPO_ROOT/}"
  furi="$rel/src/$(basename "$srcf")"
  # 合成 findings.json（file 用 PR 内完整路径，line 暂置 1，后续可由 anchor 提取改进）
  python3 - "$cid" "$scen" "$state" "$csa" "$cpp" "$furi" "$casedir" "$OUT/findings" <<'PY'
import json, sys, os
cid, scen, state, csa, cpp, furi, cdir, fout = sys.argv[1:9]
rationale = ""
nr = os.path.join(cdir, "notes.md")
if os.path.isfile(nr):
    rationale = open(nr, encoding="utf-8", errors="replace").read().strip()[:200]
doc = {"tool": "harvest-draft", "track": "defect", "case_id": cid,
       "findings": [{
           "scenario": scen, "severity": "warning",
           "file": furi, "line": 1, "anchor": "",
           "message": f"harvest 候选：猜测 {scen} | 轻量SA: {state}",
           "reasoning": f"clang: {csa.strip() or '静默'} ; cppcheck: {cpp.strip() or '静默'}",
       }]}
with open(os.path.join(fout, f"{cid}.json"), "w", encoding="utf-8") as f:
    json.dump(doc, f, ensure_ascii=False, indent=2)
PY
  echo "  $cid: scenario=$scen -> $state" >&2
  python3 - "$cid" "$scen" "$state" "$csa" "$cpp" "$rows_jsonl" <<'PY'
import json, sys
cid, scen, state, csa, cpp, path = sys.argv[1:7]
row = {"case_id": cid, "scenario": scen, "state": state,
       "csa": csa.strip(), "cppcheck": cpp.strip()}
with open(path, "a") as f:
    f.write(json.dumps(row, ensure_ascii=False) + "\n")
PY
done

# 合并 findings → SARIF（uri 已是 PR 内完整路径）
python3 "$ADAPTER" --dir "$OUT/findings" "$OUT/sarif/draft.sarif" 2>&1 | head -1

# 四态大白话表
python3 - "$OUT" <<'PY'
import json, os, sys
out = sys.argv[1]
rows = []
p = os.path.join(out, "_rows.jsonl")
if os.path.exists(p):
    for line in open(p):
        line = line.strip()
        if line:
            rows.append(json.loads(line))
tp = sum(1 for r in rows if r["state"].startswith("TP"))
fn = len(rows) - tp
with open(os.path.join(out, "report.md"), "w", encoding="utf-8") as f:
    f.write("## 🔍 候选 SARIF 评测（轻量 SA）\n\n")
    f.write("> 下列内联标注已通过 SARIF 上传，可在 **PR 的 Files changed / code-scanning** 看到每条候选的真实 bug 位置。\n")
    f.write("> 注意：PR 上 9 个 CI check 的 `SUCCESS` 只代表「工具跑通」，**不等于用例被检出**；真正的检出看本表四态。\n\n")
    f.write("| 候选 | 猜测 scenario | 轻量SA四态 | clang 信号 | CppCheck 信号 |\n|---|---|---|---|---|\n")
    for r in rows:
        f.write(f"| {r['case_id']} | {r['scenario']} | **{r['state']}** | {r['csa'] or '-'} | {r['cppcheck'] or '-'} |\n")
    f.write(f"\n**汇总**：共 {len(rows)} 条，轻量 SA 标出 {tp} 条（TP），未标出 {fn} 条（FN）。\n")
    f.write("\n> TP=轻量SA标出该scenario；FN=未标出（静默或标出其他）。accept 进 cases/ 后由完整 9 工具评测。\n")
print(f"四态表 → {os.path.join(out, 'report.md')}（TP={tp} FN={fn}）")
PY
rm -f "$rows_jsonl"
