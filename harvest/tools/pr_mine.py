#!/usr/bin/env python3
"""pr_mine.py —— harvest 的 PR 采集源（单仓版，全 CI 运行）。

爬 GitHub 开源仓历史 merged PR：取 diff + review 讨论 → 切片「修复前」函数为可编译单元
→ 产出归一化 findings（schema 与仓根 schema/findings.schema.json 同源）。

设计：仅依赖标准库 + requests。LLM-as-judge 判定「是否真 bug + scenario 家族」为占位接口
（judge_bug），默认走关键词启发式；后续接 LLM slot 不破坏契约。

默认 query：`fix in:title`（高精度的「修复类」merged PR，C/C++ 仓普遍使用 fix: 前缀）。
可在 repos.yaml 的 pr_mining.query 按仓覆盖。

新定位：harvest 是「候选线索生产线」。采集端附加三件事——
- 许可证策略：候选顶层带 license/port（direct=宽松许可可直接移植，rewrite=仅参考重写）；
- 场景配额：--max-per-scenario 按 scenario 分桶限流（CLI > config > 默认 5），防单一缺陷类型刷屏；
- contract 轨误报矿：--fp-mining 开启后对每仓额外跑一轮「修静态分析误报」PR 采集，
  产出 track_hint=contract / polarity=must_not_find 候选（与缺陷候选共用 scenario 配额桶）。

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
from datetime import datetime, timedelta

try:
    import requests
except ImportError:
    sys.stderr.write("ERROR: requests not installed (pip install requests)\n")
    sys.exit(2)

try:
    import yaml
except ImportError:
    yaml = None


# 采集预算（防卡死硬闸）：历史批扫(max_prs>900) 若无限翻页会卡死数小时。
# - SEARCH_CALL_BUDGET: 单仓 search API 调用硬上限（≈10min @2.1s 限速），到顶强制收尾
# - WALL_LIMIT_SEC: 单仓墙钟硬上限，超时 kill 当前窗、保留已采部分
# - 全局采集条数预算由 Budget.items_left 跨窗口共享（历史批扫总条数≈max_prs 而非每窗 1000）
SEARCH_CALL_BUDGET = 300
WALL_LIMIT_SEC = 1200


class Budget:
    """贯穿 fetch_merged_prs -> _sharded_search -> _search_collect 的共享预算。

    items_left: 还能采多少条（跨所有时间窗口共享，历史批扫总条数≈max_prs）
    calls_left: 还能发多少次 /search API（到顶 api_get 直接返回 None，不崩）
    deadline: 墙钟截止时间戳，超时即停
    """

    def __init__(self, items, calls=SEARCH_CALL_BUDGET, wall_sec=WALL_LIMIT_SEC):
        self.items_left = items
        self.calls_left = calls
        self.deadline = time.time() + wall_sec

    def take_item(self):
        if self.items_left <= 0:
            return False
        self.items_left -= 1
        return True

    def can_call(self):
        return self.calls_left > 0 and time.time() < self.deadline

    def consume_call(self):
        if self.calls_left > 0:
            self.calls_left -= 1

    def exhausted(self):
        return self.items_left <= 0 or self.calls_left <= 0 or time.time() >= self.deadline


# 当前采集预算（由 main 每仓设置；api_get 的 search 分支读取以强制预算闸）
_budget = None


def _ensure_utf8_streams():
    # CI runner 默认 LANG=C 时，stdout/stderr 是 ascii 编码，写中文会触发
    # UnicodeEncodeError 并被 runner 流处理放大成 RecursionError。强制 utf-8 兜底。
    for s in (sys.stdout, sys.stderr):
        if hasattr(s, "reconfigure"):
            try:
                s.reconfigure(encoding="utf-8", errors="replace")  # type: ignore[attr-defined]
            except Exception:
                pass


API = "https://api.github.com"
HEADERS = {"Accept": "application/vnd.github+json"}
TOKEN = os.environ.get("GITHUB_TOKEN", "")

# GitHub search API 限额仅 30 req/min（远严于 core 5000/min），分片翻页极易触发；
# 对 /search/* 端点做最小间隔限速，避免 403 限流拖垮整轮（符合"分几天/不触发限流"准则）。
_search_call_ts = [0.0]


def api_get(path, params=None):
    h = dict(HEADERS)
    if TOKEN:
        h["Authorization"] = f"Bearer {TOKEN}"
    if path.startswith("/search"):
        # 预算闸：calls_left 耗尽（或墙钟到）时不再发请求，直接返回 None 走空结果，不崩
        if _budget is not None and not _budget.can_call():
            sys.stderr.write("[budget] search call budget exhausted, stop\n")
            return None
        # 保证两次 search 调用间隔 ≥ 2.1s（≈28/min，留余量）
        gap = 2.1 - (time.time() - _search_call_ts[0])
        if gap > 0:
            time.sleep(gap)
        _search_call_ts[0] = time.time()
        if _budget is not None:
            _budget.consume_call()
    for attempt in range(3):
        r = requests.get(API + path, headers=h, params=params, timeout=30)
        if r.status_code == 403 and "rate limit" in r.text.lower():
            reset = int(r.headers.get("X-RateLimit-Reset", time.time() + 60))
            wait = max(1, reset - int(time.time()))
            sys.stderr.write(f"[rate-limit] waiting {min(wait,300)}s\n")
            time.sleep(min(wait, 300))
            continue
        if r.status_code != 200:
            sys.stderr.write(f"[api {r.status_code}] {path} {r.text[:120]}\n".encode("utf-8", "replace").decode("ascii", "replace"))
            return None
        return r.json()
    return None


SEARCH_CAP = 1000  # GitHub search API 单查询硬上限（total_count 最多 1000）


def _search_collect(endpoint, q, since, until, budget, per_page=30):
    """对单个时间窗口翻页收集搜索结果（items）。返回 list[dict]。遇限流重试。

    用 budget.items_left 作为全局共享条数预算（跨所有时间窗口），翻页到预算耗尽即停；
    api_get 的 search 分支已强制 calls_left / deadline 预算闸，故本函数无需单独判断墙钟。
    """
    tf = "committer-date" if endpoint == "/search/commits" else "created"  # commit 搜索用 committer-date，PR 用 created
    items_all = []
    page = 1
    window_q = q
    if since:
        window_q += f" {tf}:>={since}"
    if until:
        window_q += f" {tf}:<={until}"
    while budget.items_left > 0:
        data = api_get(endpoint, {"q": window_q, "per_page": per_page, "page": page})
        if not data or "items" not in data:
            break
        items = data["items"]
        items_all.extend(items)
        if len(items) < per_page:
            break
        if len(items_all) >= budget.items_left:
            break
        page += 1
    budget.items_left -= len(items_all)  # 全局递减，跨窗口共享
    if budget.items_left <= 0:
        sys.stderr.write("[budget] 条数预算耗尽，采集提前收尾（已收集部分保留）\n")
    return items_all


def _sharded_search(endpoint, q, since, until, max_items=SEARCH_CAP, budget=None):
    """时间二分分片绕过 search 单查询 1000 上限：某窗口 total>900 则按中点拆两段递归。
    since/until 为 YYYY-MM-DD 或 None。

    关键优化：当 max_items <= 900（日常增量采集，只要 max_prs 条）时，直接单窗口
    collect 到 max_items 即停，不二分——二分是为历史批扫(max_prs>900)绕 1000 上限
    才需要；日常场景二分会因 search 日期限定在窄窗口不可靠 + 多窗口累积翻页而卡死。

    budget：共享采集预算。递归分片时显式传入同一对象；否则取全局 _budget
    （main 每仓设置）；都没有则按 max_items 新建局部预算。预算耗尽时优雅停止，
    返回已收集部分，不崩。
    """
    # 预算解析：递归传入的共享预算 > main 设置的全局预算 > 按 max_items 新建局部预算
    budget = budget or _budget or Budget(max_items)
    if budget.exhausted():
        sys.stderr.write("[budget] 预算耗尽，停止分片采集（保留已收集部分）\n")
        return []
    tf = "committer-date" if endpoint == "/search/commits" else "created"
    probe_q = q
    if since:
        probe_q += f" {tf}:>={since}"
    if until:
        probe_q += f" {tf}:<={until}"
    probe = api_get(endpoint, {"q": probe_q, "per_page": 1})
    total = (probe or {}).get("total_count", 0) if probe else 0
    sys.stderr.write(f"[shard] {endpoint} probe total_count={total} (since={since} until={until})\n")
    if total == 0:
        return []
    # 日常采集：只要 max_items 条，直接单窗口 collect，不二分（避免卡死）
    if max_items <= 900 or not since or not until:
        return _search_collect(endpoint, q, since, until, budget)
    # 历史批扫(max_items>900)：才二分绕 1000 上限
    d0 = datetime.strptime(since, "%Y-%m-%d").date()
    d1 = datetime.strptime(until, "%Y-%m-%d").date()
    # 窗口已细化到单天（或不可逆，d0>=d1）时停止二分，直接 collect，
    # 否则 since==until 时 mid==d0 会让子窗口 (d0,d0) 永不收敛 -> RecursionError。
    if d1 <= d0:
        return _search_collect(endpoint, q, since, until, budget)
    mid = d0 + (d1 - d0) // 2
    mid_s = mid.strftime("%Y-%m-%d")
    # 右半窗口从 mid 的次日算起，确保窗口严格缩小（d1-d0==1 时拆成两个单天窗口），
    # 避免 (d0,d1) -> (d0,mid)=(d0,d0) + (mid,d1)=(d0,d1) 同窗口无限递归。
    mid_next = (mid + timedelta(days=1)).strftime("%Y-%m-%d")
    # 递归共享同一 budget：条数/调用次数/墙钟跨窗口全局递减，耗尽即优雅收尾
    return (_sharded_search(endpoint, q, since, mid_s, max_items=max_items, budget=budget)
            + _sharded_search(endpoint, q, mid_next, until, max_items=max_items, budget=budget))


def fetch_merged_prs(repo, query, max_prs, since):
    """用 search/issues 找 merged 且带修复信号的 PR（时间分片绕过 1000 上限）。
    返回 [{kind, id, title, url}]。"""
    q = f"repo:{repo} is:pr is:merged {query}".strip()
    until = datetime.now().strftime("%Y-%m-%d")
    items = _sharded_search("/search/issues", q, since, until, max_items=max_prs)
    prs = []
    for it in items:
        prs.append({"kind": "pr", "id": it["number"],
                    "title": it.get("title", ""),
                    "url": it.get("pull_request", {}).get("html_url") or it.get("html_url", "")})
        if len(prs) >= max_prs:
            break
    return prs


def fetch_commits(repo, query, max_prs, since):
    """用 repos/{repo}/commits REST 拉最近 commit（日期过滤可靠），不走 search/commits。

    注意：GitHub 的 /search/commits 对带 OR 关键词的查询不按 committer-date 过滤
    （total 恒为全仓匹配数），二分分片无意义且 collect 会海量翻页卡死。
    故改用 REST 的 /commits?since=&until= 接口，日期过滤可靠，单窗口 + max_prs 硬限。
    """
    until = datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ")
    since_iso = f"{since}T00:00:00Z" if since else None
    outs = []
    page = 1
    hard_cap = max(max_prs, 200)  # 防翻页爆炸的硬上限
    while len(outs) < hard_cap:
        params = {"per_page": 100, "page": page}
        if since_iso:
            params["since"] = since_iso
        params["until"] = until
        data = api_get(f"/repos/{repo}/commits", params)
        # REST /repos/{repo}/commits 直接返回 commit 对象 list（不是 search 的 {items: []} 包装）
        commits = data if isinstance(data, list) else []
        if not commits:
            break
        for c in commits:
            sha = c.get("sha")
            msg = (c.get("commit", {}).get("message", "") or "").split("\n")[0][:120]
            outs.append({"kind": "commit", "id": sha,
                         "title": msg, "url": c.get("html_url", "")})
            if len(outs) >= max_prs:
                break
        if len(outs) >= max_prs or len(commits) < 100:
            break
        page += 1
    return outs


def fetch_diff(repo, kind, rid):
    """取 PR 或 commit 的 files + patch（修复前/后），仅留 C/C++ 文件。
    kind='pr' → /pulls/{rid}/files；kind='commit' → /commits/{rid}。"""
    if kind == "commit":
        data = api_get(f"/repos/{repo}/commits/{rid}", {"per_page": 100})
        files = (data or {}).get("files", []) if data else []
    else:
        data = api_get(f"/repos/{repo}/pulls/{rid}/files", {"per_page": 100})
        files = data or []
    out = []
    for f in files:
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


# fp-mining（contract 轨误报矿）默认查询：「修静态分析误报」的 merged PR。
# repos.yaml pr_mining.targets 可按仓用 fp_query 覆盖。
# GitHub search 限制 AND/OR/NOT 算子最多 5 个，故保持 6 词 5 OR。
DEFAULT_FP_QUERY = ('"false positive" OR "false-positive" OR "intended behavior" '
                    'OR "not a bug" OR cppcheck OR clang-tidy')

# FP 矿 scenario 映射：PR 标题/diff 关键词 → CWE 家族（映射不了返回 None，候选省略 scenario）
FP_SCENARIO_HINTS = [
    (r"null[- ]?pointer|null pointer|null dereference|nullptr|\bnull\b", "cwe-476"),
    (r"out[- ]of[- ]bounds|\boob\b|buffer overflow|\boverflow\b", "cwe-787"),
    (r"memory leak|\bleak\b", "cwe-401"),
    (r"use[- ]after[- ]free|\buaf\b|double[- ]free", "cwe-415"),
    (r"uninit", "cwe-457"),
    (r"div(?:ide|ision)[- ]?by[- ]?zero", "cwe-369"),
    (r"integer overflow", "cwe-190"),
]


def _map_fp_scenario(text):
    """FP 矿：从 PR 标题/diff 文本映射 scenario（CWE 家族）；映射不了返回 None。"""
    for pat, scen in FP_SCENARIO_HINTS:
        if _re.search(pat, text or "", _re.I):
            return scen
    return None


def _harvest_items(items, repo, entry, args, mode, state, quota):
    """处理一批 PR/commit：拉 diff → 切修复前切片 → 判定 → scenario 配额 → 落盘候选。

    mode='defect'：缺陷轨，judge_bug 反推 scenario，polarity=must_find；
    mode='fp'：contract 轨误报矿（fp-mining），不跑 judge_bug，
        track_hint=contract + polarity=must_not_find，scenario 尽量从 PR 标题/diff
        关键词映射（映射不了省略 scenario 字段）。

    state/quota 跨轮共享：state["total"] 为跨仓候选总数（max_candidates 限流）；
    quota 为每仓 scenario 配额桶（缺陷/fp 两轮共用，防单一 scenario 刷屏，
    首跑 76 条里 72 条 nginx 的教训）。配额检查在 diff 拉取之后、落盘之前。
    返回本轮产出候选数。
    """
    per_pr_count = {}
    seen = set()  # (id, filename) 去重，避免同一文件多切片重复
    produced = 0
    for pr in items:
        if args.max_candidates and state["total"] >= args.max_candidates:
            break
        iid = pr["id"]
        if per_pr_count.get(iid, 0) >= args.max_per_pr:
            continue
        files = fetch_diff(repo, pr["kind"], iid)
        if not files:
            continue
        for fobj in files:
            if args.max_candidates and state["total"] >= args.max_candidates:
                break
            if per_pr_count.get(iid, 0) >= args.max_per_pr:
                break
            before = slice_before(fobj["patch"])
            if not before:
                continue
            dedup_key = (iid, fobj["filename"])
            if dedup_key in seen:
                continue
            seen.add(dedup_key)
            if mode == "fp":
                scenario = _map_fp_scenario(f"{pr['title']}\n{fobj['patch']}")
                severity = "medium"
                rationale = "FP 修复 PR（静态分析误报/契约安全），contract 轨误报矿候选（待 LLM/人审定）"
                anchor = None
                anchor_line = None
            else:
                is_bug, scenario, severity, rationale, anchor, anchor_line = judge_bug(fobj["patch"], pr["title"])
                if not is_bug:
                    continue
            # scenario 配额：桶满跳过后续同 scenario 候选（缺陷/fp 轮共用一桶）
            qkey = scenario or "(no-scenario)"
            if quota.get(qkey, 0) >= args.max_per_scenario:
                sys.stderr.write(f"[quota:{mode}] {repo} scenario={qkey} 桶已满({args.max_per_scenario})，跳过 {iid}/{fobj['filename']}\n")
                continue
            quota[qkey] = quota.get(qkey, 0) + 1
            # 缺陷轨 hash 输入保持原样（既有 cid 可追溯）；fp 轮加 -fp 后缀避免撞 cid
            hsrc = f"{repo}-{iid}-{fobj['filename']}" + ("-fp" if mode == "fp" else "")
            h = hashlib.sha1(hsrc.encode()).hexdigest()[:10]
            cid = f"auto-{repo.split('/')[-1]}-{h}"
            kind_tag = "commit" if pr["kind"] == "commit" else "pr"
            finding = {
                "tool": "pr-mining",
                "track": "contract" if mode == "fp" else "defect",
                "case_id": cid,
                "file": fobj["filename"],
                "function": None,
                "line": anchor_line,
                "anchor": anchor,
                "severity": severity,
                "polarity": "must_not_find" if mode == "fp" else "must_find",
                "verified": False,
                "message": f"{kind_tag.upper()} {iid} {pr['title']} :: {rationale}",
                "evidence": {
                    "source_repo": repo,
                    "kind": pr["kind"],
                    "pr": iid,
                    "pr_url": pr["url"],
                    "before_slice": before[:2000],
                    "anchor_line": anchor_line,
                },
                "raw": {"patch": fobj["patch"][:4000]},
            }
            # scenario：缺陷轨必有（judge_bug 兜底）；fp 轨映射不了则省略
            if scenario:
                finding["scenario"] = scenario
            # contract 轨标记（下游 pack_case 据此走 must_not_find 打包）
            if mode == "fp":
                finding["track_hint"] = "contract"
            # 许可证策略：license=该仓许可证，port=direct/rewrite（下游移植策略）
            if entry.get("license"):
                finding["license"] = entry["license"]
            if entry.get("port"):
                finding["port"] = entry["port"]
            with open(os.path.join(args.out, f"{cid}.json"), "w") as fh:
                json.dump(finding, fh, indent=2, ensure_ascii=False)
            state["total"] += 1
            produced += 1
            per_pr_count[iid] = per_pr_count.get(iid, 0) + 1
    return produced


def main():
    global _budget
    _ensure_utf8_streams()
    ap = argparse.ArgumentParser()
    ap.add_argument("--config")
    ap.add_argument("--repo", help="owner/repo")
    ap.add_argument("--repo-name", help="从 --config 的 repos[] 按 name 解析")
    ap.add_argument("--query", default="fix in:title")
    # 默认 None：argparse 若给 truthy 默认值会让 config 兜底永不生效。
    # 生效链：CLI 显式值 > config(pr_mining.max_prs_per_run) > 内置默认 50。
    ap.add_argument("--max-prs", type=int, default=None)
    ap.add_argument("--max-per-pr", type=int, default=3,
                    help="每 PR 最多保留的候选数（避免单个大 PR 刷屏，默认 3）")
    ap.add_argument("--max-candidates", type=int, default=0,
                    help="每仓候选总数上限（0=不限），用于历史批扫限流")
    # 生效链：CLI 显式值 > config(pr_mining.max_per_scenario) > 内置默认 5。
    ap.add_argument("--max-per-scenario", type=int, default=None,
                    help="每仓每 scenario 候选数配额（防单一缺陷类型刷屏，默认 5）")
    # 生效链：CLI 显式开启 > config(pr_mining.fp_mining) > 默认 false。
    ap.add_argument("--fp-mining", action="store_true", default=None,
                    help="开启 contract 轨误报矿：对每仓额外跑一轮「修静态分析误报」PR 采集")
    # 生效链：CLI 显式值 > config(pr_mining.since) > 内置默认 2024-01-01。
    ap.add_argument("--since", default=None)
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
            sys.stderr.write(f"ERROR: config has no repo {args.repo_name}\n")
            sys.exit(2)
        repos = [{"repo": entry["url"].split("github.com/")[-1],
                  "license": entry.get("license"), "port": entry.get("port")}]
        pm = cfg.get("pr_mining", {})
        # 优先用 targets 里该仓的 per-repo query / fp_query，否则全局 query / 内置默认
        tgt = next((t for t in pm.get("targets", []) if t.get("repo") == entry["url"].split("github.com/")[-1]), {})
        args.query = tgt.get("query", pm.get("query", args.query))
        repos[0]["fp_query"] = tgt.get("fp_query")
        # workflow --max-prs / --since 优先于 config 默认值（config 仅作兜底）
        args.max_prs = args.max_prs or pm.get("max_prs_per_run")
        args.since = args.since or pm.get("since")
        if args.max_per_scenario is None:
            args.max_per_scenario = pm.get("max_per_scenario")
        if args.fp_mining is None:
            args.fp_mining = pm.get("fp_mining", False)
    elif args.config:
        cfg = yaml_safe_load(args.config)
        pm = cfg.get("pr_mining", {})
        # repos[] 按 URL 后缀索引，给 pr_mining.targets 补许可证策略字段（license/port）
        cfg_repos = {r["url"].split("github.com/")[-1].strip(): r
                     for r in cfg.get("repos", []) if r.get("url")}
        repos = []
        for t in pm.get("targets", []):
            rentry = cfg_repos.get(t["repo"], {})
            repos.append({"repo": t["repo"], "query": t.get("query"),
                          "fp_query": t.get("fp_query"),
                          "license": rentry.get("license"), "port": rentry.get("port")})
        args.query = pm.get("query", args.query)
        args.max_prs = args.max_prs or pm.get("max_prs_per_run")
        args.since = args.since or pm.get("since")
        if args.max_per_scenario is None:
            args.max_per_scenario = pm.get("max_per_scenario")
        if args.fp_mining is None:
            args.fp_mining = pm.get("fp_mining", False)
    else:

        sys.stderr.write("ERROR: need --repo / --repo-name / --config\n")
        sys.exit(2)

    # 内置默认兜底（config 未给时用）
    if args.max_prs is None:
        args.max_prs = 50
    if args.since is None:
        args.since = "2024-01-01"
    if args.max_per_scenario is None:
        args.max_per_scenario = 5
    if args.fp_mining is None:
        args.fp_mining = False

    state = {"total": 0}  # 跨仓候选总数（max_candidates 全局限流用）
    for entry in repos:
        repo = entry["repo"]
        # 每仓一个采集预算：条数=该仓 max_prs（跨时间窗口共享），calls/wall 用类默认硬闸。
        # _sharded_search/_search_collect 全程共享，耗尽即优雅收尾（api_get 对 /search 强制闸）。
        _budget = Budget(items=args.max_prs)
        q = entry.get("query") or args.query
        sys.stderr.write(f"[pr_mine] crawling {repo} (query='{q}') ...\n")
        items = fetch_merged_prs(repo, q, args.max_prs, args.since)
        # 仓特性兜底：PR 流程缺失的仓（sqlite/postgres/linux）改爬 commit。
        # 注意用 ==0 而非 <20：fetch_merged_prs 已按 max_prs 截断，max_prs 小（如 5）
        # 时 len 必然 <20 会误触发 commit 兜底，浪费配额。只有 PR 源真为空才兜底。
        if len(items) == 0:
            sys.stderr.write(f"[pr_mine] {repo}: 0 PRs, fallback to commit source ...\n")
            items += fetch_commits(repo, q, args.max_prs, args.since)
        sys.stderr.write(f"[pr_mine] {repo}: {len(items)} items (PR+commit)\n")
        # scenario 配额桶：每仓重置；缺陷轮 + fp 轮共用（防 fp 矿也刷屏）
        quota = {}
        produced = _harvest_items(items, repo, entry, args, "defect", state, quota)
        sys.stderr.write(f"[pr_mine] {repo}: defect 轮产出 {produced} 候选\n")
        # fp-mining（contract 轨误报矿）：可选第二轮，query 换 fp_query，判定换 contract 逻辑
        if args.fp_mining:
            fq = entry.get("fp_query") or DEFAULT_FP_QUERY
            # fp 轮独立预算：与缺陷轮同一量级的 search 调用/条数硬闸
            _budget = Budget(items=args.max_prs)
            sys.stderr.write(f"[pr_mine] {repo}: fp-mining 轮 (query='{fq}') ...\n")
            fp_items = fetch_merged_prs(repo, fq, args.max_prs, args.since)
            if len(fp_items) == 0:
                sys.stderr.write(f"[pr_mine] {repo}: 0 FP-PRs, fallback to commit source ...\n")
                fp_items += fetch_commits(repo, fq, args.max_prs, args.since)
            sys.stderr.write(f"[pr_mine] {repo}: {len(fp_items)} fp items\n")
            produced_fp = _harvest_items(fp_items, repo, entry, args, "fp", state, quota)
            sys.stderr.write(f"[pr_mine] {repo}: fp 轮产出 {produced_fp} 候选\n")
    sys.stderr.write(f"[pr_mine] produced {state['total']} candidates -> {args.out}\n")
    with open(os.path.join(args.out, "_summary.json"), "w") as fh:
        json.dump({"source": "pr-mining", "count": state["total"], "repos": [r["repo"] for r in repos]}, fh, indent=2)


def yaml_safe_load(path):
    if yaml is None:
        sys.stderr.write("WARN: pyyaml not installed; pr_mining.targets need manual --repo\n")
        return {}
    with open(path) as fh:
        return yaml.safe_load(fh)


if __name__ == "__main__":
    main()
