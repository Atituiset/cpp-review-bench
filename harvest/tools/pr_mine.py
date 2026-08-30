#!/usr/bin/env python3
"""pr_mine.py —— harvest 的 PR 采集源（单仓版）。

爬 GitHub 开源仓历史 merged PR：取 diff + review 讨论 → 切片「修复前」函数为可编译单元
→ 产出归一化 findings（schema 与仓根 schema/findings.schema.json 同源）。

设计：仅依赖标准库 + requests（CI ubuntu 自带或 pip install requests）。
LLM-as-judge 判定「是否真 bug + scenario 家族」为占位接口（见 judge_bug），默认走规则启发式，
后续接 LLM slot 不破坏契约。

用法：
  python3 pr_mine.py --config ../config/repos.yaml --out /tmp/pr-findings --source pr-mining
  python3 pr_mine.py --repo curl/curl --max-prs 5 --out /tmp/out   # 单仓冒烟

产物：每个候选一个 <out>/<repo>-<pr>-<hash>.json，结构对齐 findings.schema.json。
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
# 评测/爬取用 token：CI 走 secrets.GITHUB_TOKEN（仅公开仓读，权限足够）；本地可 export GITHUB_TOKEN
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
            sys.stderr.write(f"[rate-limit] 等待 {wait}s\n")
            time.sleep(min(wait, 300))
            continue
        if r.status_code != 200:
            sys.stderr.write(f"[api {r.status_code}] {path}\n")
            return None
        return r.json()
    return None


def fetch_merged_prs(repo, query, max_prs, since):
    """用 search/issues 找 merged 且带 bug 信号的 PR。"""
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
        page += 1
    return prs


def fetch_pr_diff(repo, pr_number):
    """取 PR 的 files + patch（修复前/后）。"""
    data = api_get(f"/repos/{repo}/pulls/{pr_number}/files", {"per_page": 100})
    if not data:
        return []
    out = []
    for f in data:
        if not f.get("filename", "").endswith((".c", ".cpp", ".cc", ".h", ".hpp")):
            continue
        if f.get("patch"):
            out.append({"filename": f["filename"], "patch": f["patch"],
                        "status": f.get("status"), "sha": f.get("sha")})
    return out


def slice_before(patch):
    """从 unified diff 切出「修复前」函数段（以 @@ 上下文 + 去 '-' 行近似）。

    返回 (before_code, anchor_hint)。这是切片占位：真实实现需 cscope/clang 抽函数边界，
    此处用 diff hunk 的 '-' 行 + 上下文近似，保证可编译性由 pack_case.py 兜底。
    """
    lines = patch.splitlines()
    before = []
    for ln in lines:
        if ln.startswith("@@"):
            continue
        if ln.startswith("+") and not ln.startswith("+++"):
            continue  # 修复后，丢弃
        if ln.startswith("-") and not ln.startswith("---"):
            before.append(ln[1:])  # 修复前
        elif ln.startswith(" "):
            before.append(ln[1:])
    code = "\n".join(before).strip()
    return code, None


# 轻量 scenario 启发式（占位，后续接 LLM judge 替换）
SCENARIO_HINTS = [
    (r"\bmemcpy\s*\(|\bstrcpy\b|\bstrncpy\b", "cwe-787"),   # 危险拷贝
    (r"->\w+\s*\[|\barr\[|\bbuf\[", "cwe-787"),            # 数组下标
    (r"malloc|free|alloc", "cwe-415"),                      # 释放类
    (r"\*\s*\w+|->\w+", "cwe-476"),                         # 解引用
]


def judge_bug(patch, title):
    """占位 judge：返回 (is_bug, scenario, severity, rationale)。

    真实实现：调用 LLM slot（agent-reviewer 可复用 workflow）判「是否真 bug + CWE 家族」。
    此处用关键词启发式做 M1 冒烟，不阻断管线。
    """
    for pat, scen in SCENARIO_HINTS:
        if re.search(pat, patch or ""):
            return True, scen, "medium", f"启发式命中 {pat}"
    if re.search(r"bug|fix|leak|overflow|null", title or "", re.I):
        return True, "cwe-787", "low", "标题含缺陷信号"
    return False, None, None, "未命中启发式，需人审/LLM"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", help="harvest/config/repos.yaml（读 pr_mining 段）")
    ap.add_argument("--repo", help="单仓冒烟：owner/repo")
    ap.add_argument("--repo-name", help="从 --config 的 repos[] 按 name 解析 owner/repo")
    ap.add_argument("--query", default="label:bug")
    ap.add_argument("--max-prs", type=int, default=20)
    ap.add_argument("--since", default="2024-01-01")
    ap.add_argument("--out", required=True, help="归一化 findings 输出目录")
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
        args.query = cfg.get("pr_mining", {}).get("query", args.query)
        args.max_prs = cfg.get("pr_mining", {}).get("max_prs_per_run", args.max_prs)
        args.since = cfg.get("pr_mining", {}).get("since", args.since)
    else:
        sys.stderr.write("ERROR: 需 --repo / --repo-name / --config\n")
        sys.exit(2)

    total = 0
    for entry in repos:
        repo = entry["repo"]
        sys.stderr.write(f"[pr_mine] 爬 {repo} ...\n")
        prs = fetch_merged_prs(repo, args.query, args.max_prs, args.since)
        for pr in prs:
            files = fetch_pr_diff(repo, pr["number"])
            for fobj in files:
                before, _ = slice_before(fobj["patch"])
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
                    "function": None,           # pack_case.py 从 before 抽函数名
                    "line": None,               # 锚点由 pack_case.py 自动抓
                    "scenario": scenario,
                    "severity": severity,
                    "verified": False,
                    "message": f"PR #{pr['number']} {pr['title']} :: {rationale}",
                    "evidence": {
                        "source_repo": repo,
                        "pr": pr["number"],
                        "pr_url": pr["url"],
                        "before_slice": before[:2000],   # 截断存证
                    },
                    "raw": {"patch": fobj["patch"][:4000]},
                }
                out_path = os.path.join(args.out, f"{cid}.json")
                with open(out_path, "w") as fh:
                    json.dump(finding, fh, indent=2, ensure_ascii=False)
                total += 1
    sys.stderr.write(f"[pr_mine] 产出候选 {total} 条 → {args.out}\n")
    # 汇总（供 vote.py 消费）
    with open(os.path.join(args.out, "_summary.json"), "w") as fh:
        json.dump({"source": "pr-mining", "count": total, "repos": [r["repo"] for r in repos]}, fh, indent=2)


def yaml_safe_load(path):
    """极简 YAML 读（仅支持本仓 config 的扁平/列表结构）。避免引入 pyyaml 依赖。"""
    try:
        import yaml
        with open(path) as fh:
            return yaml.safe_load(fh)
    except ImportError:
        sys.stderr.write("WARN: 未装 pyyaml，pr_mining.targets 需手动 --repo 指定\n")
        return {}


if __name__ == "__main__":
    main()
