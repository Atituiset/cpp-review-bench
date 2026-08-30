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


# 轻量 scenario 启发式（占位，后续接 LLM judge 替换）
SCENARIO_HINTS = [
    (r"\bmemcpy\s*\(|\bstrcpy\b|\bstrncpy\b|\bmemmove\b", "cwe-787"),
    (r"->\w+\s*\[|\barr\[|\bbuf\[|\b\[[a-z_]+\]\s*=", "cwe-787"),
    (r"malloc|free|alloc|realloc|calloc", "cwe-415"),
    (r"\*\s*\w+|->\w+\s*\(|\bdelete\b", "cwe-476"),
    (r"sizeof\s*\(", "cwe-467"),
    (r"int\s+\w+\s*=|unsigned\s+\w+\s*=", "cwe-190"),
]


def judge_bug(patch, title):
    """占位 judge：返回 (is_bug, scenario, severity, rationale)。

    策略：fix-PR 且改了 C/C++ 即视为候选（is_bug=True），scenario 用关键词猜；
    真实「是否真 bug」由 LLM/人审定（见 pack_case 的 notes 留痕）。
    放宽原因：纯 label/关键词过滤会漏掉大量真实修复（如 curl 用 fix: 前缀而非 bug 标签）。
    """
    for pat, scen in SCENARIO_HINTS:
        if re.search(pat, patch or ""):
            return True, scen, "medium", f"启发式命中 {pat}"
    if re.search(r"fix|leak|overflow|null|crash|memory|corrupt|oob|ubsan|asan", title or "", re.I):
        return True, "cwe-787", "low", "标题含缺陷信号（fix/leak/overflow/...）"
    return True, "cwe-787", "low", "merged fix-PR（默认候选，待 LLM/人审定真值）"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config")
    ap.add_argument("--repo", help="owner/repo")
    ap.add_argument("--repo-name", help="从 --config 的 repos[] 按 name 解析")
    ap.add_argument("--query", default="fix in:title")
    ap.add_argument("--max-prs", type=int, default=50)
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
        for pr in prs:
            files = fetch_pr_diff(repo, pr["number"])
            if not files:
                continue
            for fobj in files:
                before = slice_before(fobj["patch"])
                if not before:
                    continue
                is_bug, scenario, severity, rationale = judge_bug(fobj["patch"], pr["title"])
                if not is_bug:
                    continue
                h = hashlib.sha1(f"{repo}-{pr['number']}-{fobj['filename']}".encode()).hexdigest()[:10]
                cid = f"auto-{repo.split('/')[-1]}-{h}"
                finding = {
                    "tool": "pr-mining",
                    "track": "defect",
                    "case_id": cid,
                    "file": fobj["filename"],
                    "function": None,
                    "line": None,
                    "scenario": scenario,
                    "severity": severity,
                    "verified": False,
                    "message": f"PR #{pr['number']} {pr['title']} :: {rationale}",
                    "evidence": {
                        "source_repo": repo,
                        "pr": pr["number"],
                        "pr_url": pr["url"],
                        "before_slice": before[:2000],
                    },
                    "raw": {"patch": fobj["patch"][:4000]},
                }
                with open(os.path.join(args.out, f"{cid}.json"), "w") as fh:
                    json.dump(finding, fh, indent=2, ensure_ascii=False)
                total += 1
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
