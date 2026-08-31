#!/usr/bin/env python3
"""normalize.py —— 多源 SARIF / 原始 findings → 归一化 findings。

输入：SA 工具产出的 SARIF（CSA/CppCheck/CodeQL 等）或 pr_mine.py 的直接 findings。
输出：对齐 schema/findings.schema.json 的归一化 findings 列表（JSON Lines 或汇总 json）。

M1 骨架：CSA/CppCheck 的 SARIF→findings 映射占位；pr-mining 的 findings 直接透传。
真实实现复用仓根 sa/adapters/*_to_findings.py（同 schema），本脚本做汇总入口。

rules.yaml（harvest/config/rules.yaml）接线状态：本脚本目前不读 rules.yaml。
  规划项（待接入）：scenario_map（规则 ID→CWE 键映射）、noise_blacklist
  （路径/规则 ID 噪声抑制进 reject 桶）、unmapped_bucket（未映射规则进人审桶）。
  vote.min_tools / vote.line_tolerance 已由 vote.py 接线，见该文件头注。
"""
import argparse
import json
import os
import sys


def _ensure_utf8_streams():
    # CI runner 默认 LANG=C 时，stdout/stderr 是 ascii 编码，写中文会触发
    # UnicodeEncodeError 并被 runner 流处理放大成 RecursionError。强制 utf-8 兜底。
    for s in (sys.stdout, sys.stderr):
        if hasattr(s, "reconfigure"):
            try:
                s.reconfigure(encoding="utf-8", errors="replace")  # type: ignore[attr-defined]
            except Exception:
                pass
def normalize_sarif(path):
    # 占位：真实实现见仓根 sa/adapters/{csa,cppcheck,clang_tidy,infer,sarif}_to_findings.py
    sys.stderr.write(f"[normalize] SARIF adapter via sa/adapters/ ({path} skipped placeholder)\n")
    return []


def normalize_prmining(path):
    with open(path) as fh:
        return [json.load(fh)]


def main():
    _ensure_utf8_streams()
    ap = argparse.ArgumentParser()
    ap.add_argument("--in-dir", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    findings = []
    # 没有 findings 产物（如本轮无任何候选 artifact）时优雅降级为空，不崩。
    if not os.path.isdir(args.in_dir):
        sys.stderr.write(f"[normalize] in-dir {args.in_dir} missing, output empty\n")
        with open(args.out, "w") as fh:
            json.dump([], fh, indent=2, ensure_ascii=False)
        return
    for fn in os.listdir(args.in_dir):
        p = os.path.join(args.in_dir, fn)
        if fn.endswith(".sarif"):
            findings += normalize_sarif(p)
        elif fn.endswith(".json") and fn != "_summary.json":
            findings += normalize_prmining(p)
    with open(args.out, "w") as fh:
        json.dump(findings, fh, indent=2, ensure_ascii=False)
    sys.stderr.write(f"[normalize] {len(findings)} findings -> {args.out}\n")


if __name__ == "__main__":
    main()
