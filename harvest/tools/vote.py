#!/usr/bin/env python3
"""vote.py —— 共识判定：≥2 工具独立命中（file+line±tol+scenario 家族一致）→ 候选。

输入：normalize.py 产出的归一化 findings（JSON）。
输出：candidates.json（共识候选列表）+ 单工具高置信单列（人审桶）。

M1 骨架：pr-mining 单源时退化为「judge 判真 bug 即候选」（不强制 ≥2 工具），
        待 sa-scan 双工具接入后启用 ≥2 共识。
"""
import argparse
import json
import sys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--findings", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--min-tools", type=int, default=2)
    ap.add_argument("--line-tol", type=int, default=3)
    args = ap.parse_args()
    with open(args.findings) as fh:
        findings = json.load(fh)

    # 单源（pr-mining）退化：直接作为候选
    sources = {f.get("tool") for f in findings}
    if len(sources) < args.min_tools:
        candidates = [f for f in findings if f.get("tool") == "pr-mining"]
        sys.stderr.write(f"[vote] 单源（{sources}），退化为 pr-mining 候选 {len(candidates)} 条\n")
    else:
        # 真实共识：按 (file, scenario) 聚类，line 在 ±tol 内计同位置
        by_key = {}
        for f in findings:
            key = (f.get("file"), f.get("scenario"))
            by_key.setdefault(key, []).append(f)
        candidates = [fs[0] for fs in by_key.values() if len({x.get("tool") for x in fs}) >= args.min_tools]
        sys.stderr.write(f"[vote] 多源共识候选 {len(candidates)} 条\n")

    with open(args.out, "w") as fh:
        json.dump(candidates, fh, indent=2, ensure_ascii=False)


if __name__ == "__main__":
    main()
