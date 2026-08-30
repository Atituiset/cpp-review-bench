#!/usr/bin/env python3
"""pr_mine.py —— harvest 的 PR 采集源（单仓版，全 CI 运行）。

爬 GitHub 开源仓历史 merged PR：取 diff + review 讨论 → 切片「修复前」函数为可编译单元
→ 产出归一化 findings（schema 与仓根 schema/findings.schema.json 同源）。

设计：仅依赖标准库 + requests。LLM-as-judge 判定「是否真 bug + scenario 家族」为占位接口
（judge_bug），默认走关键词启发式；后续接 LLM slot 不破坏契约。

默认 query：`fix in:title`（高精度的「修复类」merged PR，C/C++ 仓普遍使用 fix: 前缀）。
可在 repos.yaml 的 pr_mining.query 按仓覆盖。

用法（CI 内）：
  python3 pr_mine.py --config harvest/config/repos.yaml --repo-name curl --out /tmp/out
  python3 pr_mine.py --repo curl/curl --max-prs 50 --since 2024-01-01 --out /tmp/out
"""
import argparse
import hashlib
import json
import os
import re
import sys
import time

try:
    import requests
except ImportError:
    sys.stderr.write("ERROR: requests 未安装（pip install requests）\n")
    sys.exit(2)

API = "https://api.github.com"
HEADERS = {"Accept": "application/vnd.github+json"}
TOKEN = os.environ.get("GITHUB_TOKEN", "")


def api_get(path, params=None):
    h = dict(HEADERS)
    if TOKEN:
        h["Authorization"] = f"Bearer {TOKEN}"
    for attempt in range(3):
        r = requests.get(API + path, headers=h, params=params, timeout=30)
        if r.status_code == 403 and "rate limit" in r.text.lower():
            reset = int(r.headers.get("X-RateLimit-Reset", time.time() + 60))
            wait = max(1, reset - int(time.time()))
            sys.stderr.write(f"[rate-limit] 等待 {min(wait,300)}s\n")
            time.sleep(min(wait, 300))
            continue
        if r.status_code != 200:
            sys.stderr.write(f"[api {r.status_code}] {path} {r.text[:120]}\n")
            return None
        return r.json()
    return None


def fetch_merged_prs(repo, query, max_prs, since):
    """用 search/issues 找 merged 且带修复信号的 PR。返回 [{number,title,url}]。"""
    q = f"repo:{repo} is:pr is:merged {query}".strip()
    if since:
        q += f" created:>={since}"
    prs = []
    page = 1
    while len(prs) < max_prs:
        data = api_get("/search/issues", {"q": q, "per_page": 30, "page": page})
        if not data or "items" not in data:
            break
        items = data["items"]
        if not items:
            break
        for it in items:
            prs.append({"number": it["number"], "title": it["title"],
                        "url": it["pull_request"]["html_url"] if it.get("pull_request") else it["html_url"]})
            if len(prs) >= max_prs:
                break
        if len(items) < 30:
            break
        page += 1
    return prs


def fetch_pr_diff(repo, pr_number):
    """取 PR 的 files + patch（修复前/后），仅留 C/C++ 文件。"""
    data = api_get(f"/repos/{repo}/pulls/{pr_number}/files", {"per_page": 100})
    if not data:
        return []
    out = []
    for f in data:
        if not f.get("filename", "").endswith((".c", ".cpp", ".cc", ".cxx", ".h", ".hpp")):
            continue
        if f.get("patch"):
            out.append({"filename": f["filename"], "patch": f["patch"],
                        "status": f.get("status"), "sha": f.get("sha")})
    return out


def slice_before(patch):
    """从 unified diff 切出「修复前」函数段（去 '-' 行 + 上下文近似）。"""
    lines = patch.splitlines()
    before = []
    for ln in lines:
        if ln.startswith("@@"):
            continue
        if ln.startswith("+") and not ln.startswith("+++"):
            continue
        if ln.startswith("-") and not ln.startswith("---"):
            before.append(ln[1:])
        elif ln.startswith(" "):
            before.append(ln[1:])
    return "\n".join(before).strip()


# 从修复 diff 反推真实 bug：锚点行（修复前的 - 行）+ scenario（由 + 行的修复动作推断）
import re as _re


def _parse_hunk_lines(patch):
    """返回 [(linetype, text, old_line, new_line)]，linetype ∈ {-,+, }。"""
    out = []
    old_line = new_line = 0
    in_hunk = False
    for ln in (patch or "").splitlines():
        m = _re.match(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@", ln)
        if m:
            old_line = int(m.group(1))
            new_line = int(m.group(3))
            in_hunk = True
            continue
        if not in_hunk:
            continue
        if ln.startswith("--- ") or ln.startswith("+++ "):
            continue
        if ln.startswith("-") and not ln.startswith("---"):
            out.append(("-", ln[1:], old_line, None))
            old_line += 1
        elif ln.startswith("+") and not ln.startswith("+++"):
            out.append(("+", ln[1:], None, new_line))
            new_line += 1
        else:
            out.append((" ", ln[1:] if ln.startswith(" ") else ln, old_line, new_line))
            old_line += 1
            new_line += 1
    return out


# 修复动作 → scenario 推断（看 + 行做了什么防护）
FIX_PATTERNS = [
    (r"if\s*\([^)]*(==\s*NULL|!=\s*NULL|nullptr|!\s*\w+\s*\)|assert\s*\()",
     "cwe-476", "修复前缺判空即解引用（加 null 检查）"),
    (r"&&\s*\w+\s*->|&&\s*\w+\s*\[|&&\s*\(?\s*\w+\s*\)?\s*&&|\|\|\s*\w+\s*->",
     "cwe-476", "修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）"),
    (r"if\s*\([^)]*(<|<=|>|>=)\s*\w*(size|len|count|idx|index|nb|num)",
     "cwe-787", "修复前越界访问（加边界/长度检查）"),
    (r"if\s*\([^)]*(<|<=|>|>=)", "cwe-787", "修复前越界访问（加边界检查）"),
    (r"free\s*\(\s*\w+\s*\)\s*(==\s*NULL|!=\s*NULL|nullptr)", "cwe-415",
     "修复前释放后未置空（释放守卫/双重释放）"),
    (r"free\s*\(", "cwe-415", "修复前释放/双重释放（加释放守卫）"),
    (r"memcpy|strncpy|memmove|snprintf", "cwe-787", "修复前缓冲区溢出（加长度约束）"),
    (r"->\w+\s*\(|\*\w+\s*\(", "cwe-476", "修复前解引用（疑似缺判空）"),
]


def judge_bug(patch, title):
    """从修复 diff 反推候选：返回 (is_bug, scenario, severity, rationale, anchor, anchor_line)。

    - anchor：修复前的 - 行里最可能是 bug 的那行（含解引用/越界/释放信号）
    - scenario：由 + 行的修复动作推断（加判空→cwe-476，加边界→cwe-787，加释放守卫→cwe-415）
    - anchor_line：该 - 行在切片中的真实行号（用于 SARIF 标注 + src 对齐）

    真实「是否真 bug」仍由 LLM/人审定；此处给的是有依据的猜测，而非瞎猜。
    """
    hunk = _parse_hunk_lines(patch)
    minus = [h for h in hunk if h[0] == "-"]
    plus = [h for h in hunk if h[0] == "+"]
    plus_text = "\n".join(p[1] for p in plus)
    # 1) 由 + 行修复动作推断 scenario
    scenario = None
    sev = "medium"
    rationale = "merged fix-PR（默认候选，待 LLM/人审定真值）"
    for pat, scen, why in FIX_PATTERNS:
        if _re.search(pat, plus_text):
            scenario = scen
            rationale = f"PR 修复动作推断：{why}"
            break
    if not scenario:
        # 退路：标题含缺陷信号
        if _re.search(r"fix|leak|overflow|null|crash|memory|corrupt|oob|ubsan|asan", title or "", _re.I):
            scenario, rationale = "cwe-476", "标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作"
        else:
            scenario, rationale = "cwe-787", "merged fix-PR（默认候选，待 LLM/人审定真值）"
    # 2) 由 - 行找 bug 锚点：优先含解引用/越界/释放信号
    anchor = None
    anchor_line = None
    for pat, _, _ in FIX_PATTERNS:
        for ltype, text, old_line, _ in minus:
            if _re.search(pat, text):
                anchor = text.strip()
                anchor_line = old_line
                break
        if anchor:
            break
    # 退路：第一条非空 - 代码行
    if not anchor:
        for ltype, text, old_line, _ in minus:
            t = text.strip()
            if t and not t.startswith(("{", "}", "#", "//", "/*", "*")):
                anchor = t
                anchor_line = old_line
                break
    return True, scenario, sev, rationale, anchor, anchor_line


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config")
    ap.add_argument("--repo", help="owner/repo")
    ap.add_argument("--repo-name", help="从 --config 的 repos[] 按 name 解析")
    ap.add_argument("--query", default="fix in:title")
    ap.add_argument("--max-prs", type=int, default=50)
    ap.add_argument("--max-per-pr", type=int, default=3,
                    help="每 PR 最多保留的候选数（避免单个大 PR 刷屏，默认 3）")
    ap.add_argument("--max-candidates", type=int, default=0,
                    help="每仓候选总数上限（0=不限），用于历史批扫限流")
    ap.add_argument("--since", default="2024-01-01")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)

    repos = []
    if args.repo:
        repos = [{"repo": args.repo}]
    elif args.repo_name:
        cfg = yaml_safe_load(args.config) if args.config else {}
        entry = next((r for r in cfg.get("repos", []) if r["name"] == args.repo_name), None)
        if not entry:
            sys.stderr.write(f"ERROR: config 中无 repo {args.repo_name}\n")
            sys.exit(2)
        repos = [{"repo": entry["url"].split("github.com/")[-1]}]
        pm = cfg.get("pr_mining", {})
        args.query = pm.get("query", args.query)
        args.max_prs = pm.get("max_prs_per_run", args.max_prs)
        args.since = pm.get("since", args.since)
    elif args.config:
        cfg = yaml_safe_load(args.config)
        repos = [{"repo": t["repo"]} for t in cfg.get("pr_mining", {}).get("targets", [])]
        pm = cfg.get("pr_mining", {})
        args.query = pm.get("query", args.query)
        args.max_prs = pm.get("max_prs_per_run", args.max_prs)
        args.since = pm.get("since", args.since)
    else:
        sys.stderr.write("ERROR: 需 --repo / --repo-name / --config\n")
        sys.exit(2)

    total = 0
    for entry in repos:
        repo = entry["repo"]
        sys.stderr.write(f"[pr_mine] 爬 {repo} (query='{args.query}') ...\n")
        prs = fetch_merged_prs(repo, args.query, args.max_prs, args.since)
        sys.stderr.write(f"[pr_mine] {repo}: 命中 PR {len(prs)} 条\n")
        per_pr_count = {}
        seen = set()  # (pr_number, filename) 去重，避免同一文件多切片重复
        for pr in prs:
            if args.max_candidates and total >= args.max_candidates:
                break
            prn = pr["number"]
            if per_pr_count.get(prn, 0) >= args.max_per_pr:
                continue
            files = fetch_pr_diff(repo, prn)
            if not files:
                continue
            for fobj in files:
                if args.max_candidates and total >= args.max_candidates:
                    break
                if per_pr_count.get(prn, 0) >= args.max_per_pr:
                    break
                before = slice_before(fobj["patch"])
                if not before:
                    continue
                dedup_key = (prn, fobj["filename"])
                if dedup_key in seen:
                    continue
                seen.add(dedup_key)
                is_bug, scenario, severity, rationale, anchor, anchor_line = judge_bug(fobj["patch"], pr["title"])
                if not is_bug:
                    continue
                h = hashlib.sha1(f"{repo}-{prn}-{fobj['filename']}".encode()).hexdigest()[:10]
                cid = f"auto-{repo.split('/')[-1]}-{h}"
                finding = {
                    "tool": "pr-mining",
                    "track": "defect",
                    "case_id": cid,
                    "file": fobj["filename"],
                    "function": None,
                    "line": anchor_line,
                    "anchor": anchor,
                    "scenario": scenario,
                    "severity": severity,
                    "verified": False,
                    "message": f"PR #{pr['number']} {pr['title']} :: {rationale}",
                    "evidence": {
                        "source_repo": repo,
                        "pr": pr["number"],
                        "pr_url": pr["url"],
                        "before_slice": before[:2000],
                        "anchor_line": anchor_line,
                    },
                    "raw": {"patch": fobj["patch"][:4000]},
                }
                with open(os.path.join(args.out, f"{cid}.json"), "w") as fh:
                    json.dump(finding, fh, indent=2, ensure_ascii=False)
                total += 1
                per_pr_count[prn] = per_pr_count.get(prn, 0) + 1
    sys.stderr.write(f"[pr_mine] 产出候选 {total} 条 → {args.out}\n")
    with open(os.path.join(args.out, "_summary.json"), "w") as fh:
        json.dump({"source": "pr-mining", "count": total, "repos": [r["repo"] for r in repos]}, fh, indent=2)


def yaml_safe_load(path):
    try:
        import yaml
        with open(path) as fh:
            return yaml.safe_load(fh)
    except ImportError:
        sys.stderr.write("WARN: 未装 pyyaml，pr_mining.targets 需手动 --repo 指定\n")
        return {}


if __name__ == "__main__":
    main()
