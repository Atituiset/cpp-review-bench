#!/usr/bin/env bash
# run_eval_inbox.sh —— 对 harvest/inbox/draft 候选跑轻量 SA 评测，产出「该真实 bug 片段能否被标出」报表。
#
# 设计：inbox 候选是外部开源仓的函数片段，无法在 bench 完整构建环境编译（缺外部仓头文件/依赖），
# 故用单文件 SA（clang --analyze / cppcheck）评估「片段自身能否被 SA 标出缺陷信号」——
# 这是 M2 评测环的务实形态；确认进 cases/ 后再走完整 9 工具（仓根 ci.yml）。
#
# 命中口径（对齐 tools/eval.py 的 L1）：file+anchor 命中——SA finding 的 anchor 去空白后
# 与 golden anchor 互为子串。本脚本 SA finding 没有 anchor 文本，用「SA 命中行对应的源码行
# 文本」作为 finding anchor 的最近似等价（同一文件内判定，file 恒等；SA finding 无 scenario
# 字段，故不校验 scenario 家族——这两点是与 eval.py L1 的已知差异）。
# 注意：旧版用 scenario 数字（如 476）grep checker 名，几乎不可能命中，已废弃。
#
# 输入：
#   $1 = 仓根（REPO_ROOT）
#   $2 = inbox 根（harvest/inbox）
#   $3 = 报表输出目录
# 输出：
#   $3/eval_inbox_report.json  每候选的 {case_id, scenario, csa_hits, cppcheck_hits, verdict}
#   $3/eval_inbox_report.md    人审用摘要（附 PR body）
set -euo pipefail
# 强制 UTF-8 流编码：CI runner 默认 LANG=C，Python 往 stderr/stdout 写中文会触发
# UnicodeEncodeError -> runner 流处理放大成 RecursionError 使步骤崩溃。
export PYTHONIOENCODING=utf-8
export LANG=C.UTF-8 LC_ALL=C.UTF-8

REPO_ROOT="${1:?用法: $0 <repo_root> <inbox_root> <out_dir>}"
INBOX="${2:?}"
OUT="${3:?}"
mkdir -p "$OUT"

# 工具缺失属硬失败（区别于「跑通但零命中」的合法 sa-silent 结果）
for bin in clang cppcheck; do
  if ! command -v "$bin" >/dev/null 2>&1; then
    echo "[ERROR] 找不到 $bin（工具缺失，CI 应变红）" >&2
    exit 127
  fi
done

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

echo "=== 评测 inbox 候选（轻量单文件 SA）==="
rows_jsonl="$OUT/_rows.jsonl"
: > "$rows_jsonl"
find "$INBOX/draft" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | while read -r casedir; do
  cid="$(basename "$casedir")"
  # golden schema：must_find 嵌套在 expected 下；旧草稿曾写顶层，用 .get 链 + 顶层兜底容错
  # （|| true：候选草稿 golden 可能残缺，属数据问题非硬失败，降级为 unknown 继续）
  golden_info="$(python3 -c "
import json
g = json.load(open('$casedir/golden.json'))
mf = (g.get('expected') or {}).get('must_find') or g.get('must_find') or [{}]
print(mf[0].get('scenario') or 'unknown')
print((mf[0].get('anchor') or '').replace(chr(10), ' '))
" 2>/dev/null || true)"
  scen="$(printf '%s\n' "$golden_info" | sed -n 1p)"
  ganchor="$(printf '%s\n' "$golden_info" | sed -n 2p)"
  [ -z "$scen" ] && scen=unknown
  # （|| true：src 目录缺失时 find 非零，属「无源文件」情形，srcf 置空走下方 sa-silent 分支）
  srcf="$(find "$casedir/src" \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' \) 2>/dev/null | head -1 || true)"
  csa="" cpp=""
  if [ -n "$srcf" ]; then
    csa="$(csa_hit "$srcf")"
    cpp="$(cppcheck_hit "$srcf")"
  fi
  # verdict：file+anchor 命中判定（见文件头部口径说明），并写一行 JSONL
  CID="$cid" SCEN="$scen" GANCHOR="$ganchor" SRCF="$srcf" \
  CSA="$csa" CPP="$cpp" ROWS="$rows_jsonl" python3 <<'PY'
import json, os, re
cid = os.environ["CID"]; scen = os.environ["SCEN"]
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
    v = "detected-by-csa"
elif anchor_hit(cpp):
    v = "detected-by-cppcheck"
elif csa or cpp:
    v = "sa-flagged-other"
else:
    v = "sa-silent"
row = {"case_id": cid, "scenario": scen,
       "csa_hits": [c for _, c in csa], "cppcheck_hits": [c for _, c in cpp], "verdict": v}
with open(os.environ["ROWS"], "a") as f:
    f.write(json.dumps(row, ensure_ascii=False) + "\n")
print(f"  {cid}: scenario={scen} -> {v}")
PY
done

python3 - "$OUT" <<'PY'
import json, collections, os, sys
out = sys.argv[1]
rows = []
p = os.path.join(out, "_rows.jsonl")
if os.path.exists(p):
    with open(p) as f:
        for line in f:
            line = line.strip()
            if line:
                rows.append(json.loads(line))
vc = collections.Counter(r["verdict"] for r in rows)
summary = {"total": len(rows), "verdicts": dict(vc)}
with open(os.path.join(out, "eval_inbox_report.json"), "w") as f:
    json.dump({"summary": summary, "cases": rows}, f, indent=2, ensure_ascii=False)
with open(os.path.join(out, "eval_inbox_report.md"), "w") as f:
    f.write(f"# inbox 候选 SA 评测报表（{len(rows)} 条）\n\n")
    f.write("| 候选 | scenario | CSA | CppCheck | 结论 |\n|---|---|---|---|---|\n")
    for r in rows:
        f.write(f"| {r['case_id']} | {r['scenario']} | {';'.join(r['csa_hits']) or '-'} | {';'.join(r['cppcheck_hits']) or '-'} | {r['verdict']} |\n")
    f.write(f"\n**汇总**：{json.dumps(summary['verdicts'], ensure_ascii=False)}\n")
    f.write("\n> 说明：轻量单文件 SA（clang --analyze + cppcheck）仅评估片段自身能否被标出；")
    f.write("命中判定为 file+anchor 口径（对齐 eval.py L1，SA 命中行源码文本与 golden anchor 去空白互为子串）。")
    f.write("确认进 cases/ 后走完整 9 工具（仓根 ci.yml）。\n")
print("报表 →", os.path.join(out, "eval_inbox_report.md"))
PY
rm -f "$rows_jsonl"
