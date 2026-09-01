#!/usr/bin/env python3
"""vote.py —— 共识判定：≥N 工具独立命中（file+line±tol+scenario 家族一致）→ 候选。

输入：normalize.py 产出的归一化 findings（JSON）。
输出：candidates.json（共识候选列表）+ 单工具高置信单列（人审桶）。

M1 骨架：pr-mining 单源时退化为「judge 判真 bug 即候选」（不强制 ≥2 工具），
        待 sa-scan 双工具接入后启用 ≥2 共识。

rules.yaml（harvest/config/rules.yaml）接线状态：
  已生效：vote.min_tools（--min-tools 未显式传时作默认值，CLI 显式传参优先）、
          vote.line_tolerance（--line-tol 未显式传时作默认值，同上）。
  规划项（尚未接线，见 normalize.py 头注）：scenario_map、noise_blacklist、
          vote.single_tool_high_conf、unmapped_bucket。
"""
import argparse
import json
import os
import sys

try:
    import yaml
except ImportError:
    yaml = None

# 默认规则文件：相对本脚本定位（harvest/tools/../config/rules.yaml），与 CWD 无关
DEFAULT_RULES = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "config", "rules.yaml")


def _ensure_utf8_streams():
    # CI runner 默认 LANG=C 时，stdout/stderr 是 ascii 编码，写中文会触发
    # UnicodeEncodeError 并被 runner 流处理放大成 RecursionError。强制 utf-8 兜底。
    for s in (sys.stdout, sys.stderr):
        if hasattr(s, "reconfigure"):
            try:
                s.reconfigure(encoding="utf-8", errors="replace")  # type: ignore[attr-defined]
            except Exception:
                pass


def _load_vote_rules(path):
    """读 rules.yaml 的 vote 段；文件缺失/pyyaml 未装时返回 {}（不崩，走内置默认）。"""
    if yaml is None or not os.path.isfile(path):
        return {}
    try:
        with open(path) as fh:
            return (yaml.safe_load(fh) or {}).get("vote", {}) or {}
    except Exception as e:
        sys.stderr.write(f"[vote] WARN: rules 读取失败 {path}: {e}，走内置默认\n")
        return {}


def _cluster_positions(fs, tol):
    """同一 (file, scenario) 下按 line 做 ±tol 连通聚类：line 差 ≤tol 的 finding 归同一位置簇。

    无 line 的 finding 各自单独成簇（保守，不与任何有位置者合并）。
    返回 list[list[finding]]。
    """
    def has_line(f):
        return isinstance(f.get("line"), int)
    scored = sorted((f for f in fs if has_line(f)), key=lambda f: f["line"])
    noline = [f for f in fs if not has_line(f)]
    clusters = [[f] for f in noline]
    for f in scored:
        placed = False
        for c in clusters:
            if any(has_line(x) and abs(x["line"] - f["line"]) <= tol for x in c):
                c.append(f)
                placed = True
                break
        if not placed:
            clusters.append([f])
    return clusters


def main():
    _ensure_utf8_streams()
    ap = argparse.ArgumentParser()
    ap.add_argument("--findings", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--rules", default=DEFAULT_RULES, help="rules.yaml 路径（默认 harvest/config/rules.yaml）")
    # 默认 None：CLI 显式传参 > rules.yaml vote.* > 内置默认（2 / 3）
    ap.add_argument("--min-tools", type=int, default=None)
    ap.add_argument("--line-tol", type=int, default=None)
    args = ap.parse_args()
    rules = _load_vote_rules(args.rules)
    if args.min_tools is None:
        args.min_tools = int(rules.get("min_tools", 2))
    if args.line_tol is None:
        args.line_tol = int(rules.get("line_tolerance", 3))
    with open(args.findings) as fh:
        findings = json.load(fh)

    # 单源（pr-mining）退化：直接作为候选
    sources = {f.get("tool") for f in findings}
    if len(sources) < args.min_tools:
        candidates = [f for f in findings if f.get("tool") == "pr-mining"]
        sys.stderr.write(f"[vote] single-source {sources}, fallback to pr-mining candidates: {len(candidates)}\n")
    else:
        # 真实共识：按 (file, scenario) 聚类，line 在 ±tol 内计同位置；
        # 同一位置簇内去重工具数 ≥ min_tools 才算共识（避免两工具对不同行报同一场景误判）。
        by_key = {}
        for f in findings:
            key = (f.get("file"), f.get("scenario"))
            by_key.setdefault(key, []).append(f)
        candidates = []
        for fs in by_key.values():
            for cluster in _cluster_positions(fs, args.line_tol):
                if len({x.get("tool") for x in cluster}) >= args.min_tools:
                    candidates.append(cluster[0])
                    break
        sys.stderr.write(f"[vote] multi-source consensus candidates: {len(candidates)}\n")

    with open(args.out, "w") as fh:
        json.dump(candidates, fh, indent=2, ensure_ascii=False)


if __name__ == "__main__":
    main()
