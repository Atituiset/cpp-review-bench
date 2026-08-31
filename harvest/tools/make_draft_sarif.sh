#!/usr/bin/env bash
# make_draft_sarif.sh —— 对 harvest/inbox/draft 候选跑轻量 SA，产出：
#   1) 每个候选的 findings.json（合成：golden 猜测 scenario + 轻量 SA 是否标出）
#   2) 合并 SARIF（uri 指向 draft 内 src 文件 → PR 内联标注）
#   3) 四态大白话表（PR 评论用）
#
# 用法: make_draft_sarif.sh <repo_root> <inbox_root> <out_dir>
# 依赖: clang --analyze, cppcheck, python3 sa/adapters/findings_to_sarif.py
#
# 命中口径（与 sa/runners/run_eval_inbox.sh 一致，对齐 eval.py L1）：file+anchor——
# SA 命中行的源码行文本去空白后与 golden anchor 互为子串。
# 旧版用 scenario 数字（如 476）grep checker 名，几乎不可能命中，已废弃。
set -u
# 强制 UTF-8 流编码：CI runner 默认 LANG=C，Python 往 stderr/stdout 写中文会触发
# UnicodeEncodeError -> runner 流处理放大成 RecursionError 使步骤崩溃。
export PYTHONIOENCODING=utf-8
export LANG=C.UTF-8 LC_ALL=C.UTF-8

REPO_ROOT="$(cd "${1:?用法: $0 <repo_root> <inbox_root> <out_dir>}" && pwd)"
INBOX="$(cd "${2:?}" && pwd)"
OUT="${3:?}"
mkdir -p "$OUT/findings" "$OUT/sarif"

ADAPTER="$REPO_ROOT/sa/adapters/findings_to_sarif.py"

# 输出 "行号<TAB>checker名" 每行一条；plist 用 python plistlib（标准库）解析
csa_hit() {
  local f="$1" plist
  plist="$(mktemp --suffix=.plist)"
  clang --analyze -Xanalyzer -analyzer-output=plist "$f" -o "$plist" >/dev/null 2>&1 || true
  if [ -s "$plist" ]; then
    python3 - "$plist" <<'PY'
import plistlib, sys
try:
    with open(sys.argv[1], "rb") as fh:
        doc = plistlib.load(fh)
except Exception:
    sys.exit(0)
for d in doc.get("diagnostics", []):
    line = (d.get("location") or {}).get("line", 0)
    # check_name 是规则 ID（如 core.NullDereference），type 是描述文本；优先规则 ID
    print(f"{line}\t{d.get('check_name') or d.get('type', '')}")
PY
  fi
  rm -f "$plist"
}
# 输出 "行号<TAB>error-id" 每行一条。
# 注意：cppcheck --xml 的 XML 报告写到 stderr（官方行为），必须 2>&1 >/dev/null
# 把 XML 引进管道；直接 2>/dev/null 会把报告丢弃导致恒为空（旧版 bug）。
cppcheck_hit() {
  cppcheck --enable=warning,style,performance,portability --xml --xml-version=2 "$1" 2>&1 >/dev/null \
    | python3 -c '
import re, sys
xml = sys.stdin.read()
for m in re.finditer(r"<error\b[^>]*?\bid=\"([^\"]+)\"[^>]*?(?:/>|>(.*?)</error>)", xml, re.S):
    eid, body = m.group(1), m.group(2) or ""
    lm = re.search(r"<location\b[^>]*?\bline=\"(\d+)\"", body)
    print(f"{lm.group(1) if lm else 0}\t{eid}")
'
}

echo "=== 生成 draft 候选 SARIF + 四态表 ==="
rows_jsonl="$OUT/_rows.jsonl"
: > "$rows_jsonl"

find "$INBOX/draft" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | while read -r casedir; do
  cid="$(basename "$casedir")"
  # 读取 golden 猜测 scenario + 真实 bug 锚点（来自修复 diff 反推）
  # golden schema：must_find 嵌套在 expected 下；旧草稿曾写顶层，用 .get 链 + 顶层兜底容错
  golden_info="$(python3 -c "
import json
g=json.load(open('$casedir/golden.json'))
mf=((g.get('expected') or {}).get('must_find') or g.get('must_find') or [{}])[0]
print(mf.get('scenario') or 'unknown')
print(mf.get('line') or 1)
print((mf.get('anchor') or '').replace(chr(10),' '))
" 2>/dev/null)"
  scen="$(printf '%s\n' "$golden_info" | sed -n 1p)"
  anchorline="$(printf '%s\n' "$golden_info" | sed -n 2p)"
  ganchor="$(printf '%s\n' "$golden_info" | sed -n 3p)"
  [ -z "$scen" ] && scen=unknown
  srcf="$(find "$casedir/src" \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' \) 2>/dev/null | head -1)"
  csa="" cpp=""
  if [ -n "$srcf" ]; then
    csa="$(csa_hit "$srcf")"
    cpp="$(cppcheck_hit "$srcf")"
  fi
  # 四态判定：file+anchor 口径（见文件头部说明；SA 命中行源码文本与 golden anchor 互为子串）
  read state sa < <(GANCHOR="$ganchor" SRCF="$srcf" CSA="$csa" CPP="$cpp" python3 <<'PY'
import os, re
ganchor = re.sub(r"\s+", "", os.environ.get("GANCHOR", ""))
srcf = os.environ.get("SRCF", "")

def parse_hits(s):
    out = []
    for ln in s.splitlines():
        parts = ln.split("\t")
        if len(parts) == 2 and parts[0].isdigit():
            out.append((int(parts[0]), parts[1]))
    return out

csa = parse_hits(os.environ.get("CSA", ""))
cpp = parse_hits(os.environ.get("CPP", ""))

lines = []
if srcf and os.path.isfile(srcf):
    with open(srcf, encoding="utf-8", errors="replace") as fh:
        lines = fh.read().splitlines()

# eval.py L1 近似：finding.anchor := SA 命中行的源码行文本，去空白后与 golden anchor 互为子串
def anchor_hit(hits):
    if not ganchor:
        return False
    for ln, _ in hits:
        if 1 <= ln <= len(lines):
            fa = re.sub(r"\s+", "", lines[ln - 1])
            if fa and (ganchor in fa or fa in ganchor):
                return True
    return False

if anchor_hit(csa):
    print("TP(CSA标出)", "csa")
elif anchor_hit(cpp):
    print("TP(CppCheck标出)", "cppcheck")
elif csa or cpp:
    print("FN(标出其他)", "other")
else:
    print("FN(静默)", "none")
PY
)
  # 合成 findings.json（file 用 PR 内完整路径，line 用真实 bug 行）
  rel="${casedir#$REPO_ROOT/}"
  furi="$rel/src/$(basename "$srcf")"
  # 用环境变量传参，避免 sys.argv 个数不匹配
  CID="$cid" SCEN="$scen" STATE="$state" CSA="$csa" CPP="$cpp" \
  FURI="$furi" ANCHORLINE="$anchorline" CDIR="$casedir" FOUT="$OUT/findings" \
  python3 <<'PY'
import json, sys, os
cid=os.environ['CID']; scen=os.environ['SCEN']; state=os.environ['STATE']
csa=os.environ['CSA']; cpp=os.environ['CPP']; furi=os.environ['FURI']
anchorline=os.environ['ANCHORLINE']; cdir=os.environ['CDIR']; fout=os.environ['FOUT']
ev = {}
nr = os.path.join(cdir, "notes.md")
import re as _re
if os.path.isfile(nr):
    txt = open(nr, encoding="utf-8", errors="replace").read()
    m = _re.search(r"源 PR: #(\d+) \((https?://[^)]+)\)", txt)
    if m:
        ev = {"pr": int(m.group(1)), "pr_url": m.group(2)}
doc = {"tool": "harvest-draft", "track": "defect", "case_id": cid,
       "findings": [{
           "scenario": scen, "severity": "warning",
           "file": furi, "line": int(anchorline or 1), "anchor": "",
           "message": f"候选初判 {scen}（待 LLM/人审定，非真值）@ {furi}:{anchorline}；源PR#{ev.get('pr')}",
           "reasoning": f"采集: pr-mining; bug锚点行 {anchorline}; 轻量SA: {state}",
       }]}
with open(os.path.join(fout, f"{cid}.json"), "w", encoding="utf-8") as f:
    json.dump(doc, f, ensure_ascii=False, indent=2)
PY
  echo "  $cid: scenario=$scen line=$anchorline -> $state" >&2
  CID="$cid" SCEN="$scen" STATE="$state" CSA="$csa" CPP="$cpp" \
  ANCHORLINE="$anchorline" CDIR="$casedir" ROWS="$rows_jsonl" \
  python3 <<'PY'
import json, sys, re as _re, os
cid=os.environ['CID']; scen=os.environ['SCEN']; state=os.environ['STATE']
csa=os.environ['CSA']; cpp=os.environ['CPP']; anchorline=os.environ['ANCHORLINE']
cdir=os.environ['CDIR']; path=os.environ['ROWS']
ev = {}
dep = ""
nr = os.path.join(cdir, "notes.md")
if os.path.isfile(nr):
    txt = open(nr, encoding="utf-8", errors="replace").read()
    m = _re.search(r"源 PR: #(\d+) \((https?://[^)]+)\)", txt)
    if m:
        ev = {"pr": int(m.group(1)), "pr_url": m.group(2)}
    # 外部依赖数（pack_case 写在 notes 溯源表；旧草稿无此行则留空）
    md = _re.search(r"外部依赖数（dep_count）\s*\|\s*(\d+)", txt)
    if md:
        dep = md.group(1)
with open(path, "a") as f:
    f.write(json.dumps({"case_id": cid, "scenario": scen, "state": state,
        "csa": csa.strip(), "cppcheck": cpp.strip(), "anchorline": anchorline,
        "dep_count": dep,
        "pr": ev.get("pr"), "pr_url": ev.get("pr_url")}, ensure_ascii=False) + "\n")
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
    f.write("## 🔍 候选溯源总览（草稿，待审）\n\n")
    f.write("> 每条候选来自真实已合并 fix-PR，采集信号 = 标题/修复 diff 含缺陷特征。\n")
    f.write("> **scenario 为候选初判（非真值）**，待你正式仓手动触发 LLM 评审（agent-reviewer）后写入 golden。\n")
    f.write("> 四态评测（9 工具 PASS/FN/FP/EXTRA）在 `/case accept` 进 cases/ 后由 `ci.yml`+`eval.py` 产生。\n\n")
    f.write("| 候选 | 初判scenario | 锚点行 | dep | 采集工具 | 源PR | accept命令 |\n|---|---|---|---|---|---|---|\n")
    for r in rows:
        f.write(f"| {r['case_id']} | {r['scenario']}（待定） | {r.get('anchorline') or '-'} | {r.get('dep_count') or '-'} | pr-mining | "
                f"[PR#{r.get('pr')}]({r.get('pr_url')}) | `/case accept {r['case_id']}` |\n")
    f.write(f"\n**共 {len(rows)} 条候选**（curl/redis 等真实仓 fix-PR 爬取）。\n")
    f.write("\n> 上表 scenario 列是采集阶段的启发式初判，可能不准（见 notes.md 真实修复 diff）。accept 后用 9 工具实测 + 你手动 LLM 评审定真值。\n")
print(f"四态表 → {os.path.join(out, 'report.md')}（TP={tp} FN={fn}）")
PY
rm -f "$rows_jsonl"
