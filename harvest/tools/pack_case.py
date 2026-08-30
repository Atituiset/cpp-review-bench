#!/usr/bin/env python3
"""pack_case.py —— 候选 finding → 用例五文件草稿，写入 harvest/inbox/<id>/。

五文件：src/（enclosing file 原样或 before 切片）、CMakeLists.txt、golden.json 草稿、
contract.yaml（空）、notes.md（来源+vote 明细+证据链）。

M1 骨架：pr-mining 候选用 evidence.before_slice 落 src/ 草稿；真实函数边界抽取留给 v0.2。
"""
import argparse
import json
import os
import sys


def pack(finding, inbox_root):
    cid = finding["case_id"]
    d = os.path.join(inbox_root, "draft", cid)
    os.makedirs(os.path.join(d, "src"), exist_ok=True)
    # src 草稿：before 切片（占位，保证可编译性由 v0.2 补全函数头）
    before = (finding.get("evidence", {}) or {}).get("before_slice", "")
    with open(os.path.join(d, "src", os.path.basename(finding.get("file", "snippet.c"))), "w") as fh:
        fh.write(f"// AUTO-DRAFT from {finding['evidence'].get('source_repo')} PR #{finding['evidence'].get('pr')}\n")
        fh.write(before + "\n")
    # CMakeLists 占位：单文件 -c 编译目标
    with open(os.path.join(d, "CMakeLists.txt"), "w") as fh:
        fh.write(f"# AUTO-DRAFT; 真实构建片段由 v0.2 补全\nadd_library({cid}_draft STATIC src/{os.path.basename(finding.get('file','snippet.c'))})\n")
    # golden 草稿
    golden = {
        "must_find": [{
            "scenario": finding.get("scenario"),
            "file": f"src/{os.path.basename(finding.get('file','snippet.c'))}",
            "anchor": None,           # 人审/自动抓
            "function": finding.get("function"),
            "rationale": finding.get("message"),
            "severity": finding.get("severity"),
        }],
        "must_not_find": [],
    }
    with open(os.path.join(d, "golden.json"), "w") as fh:
        json.dump(golden, fh, indent=2, ensure_ascii=False)
    # contract.yaml 空（confirm-fp 时填）
    with open(os.path.join(d, "contract.yaml"), "w") as fh:
        fh.write("# 人审判 FP 时填写：该 FP 因何契约成立（exemption_pattern）\n")
    # notes 骨架
    with open(os.path.join(d, "notes.md"), "w") as fh:
        fh.write(f"# {cid}\n\n- 来源仓: {finding['evidence'].get('source_repo')}\n")
        fh.write(f"- PR: #{finding['evidence'].get('pr')} ({finding['evidence'].get('pr_url')})\n")
        fh.write(f"- 命中工具: {finding.get('tool')}\n- scenario: {finding.get('scenario')}\n")
        fh.write(f"- judge: {finding.get('message')}\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--candidates", required=True)
    ap.add_argument("--inbox", required=True)
    args = ap.parse_args()
    with open(args.candidates) as fh:
        cands = json.load(fh)
    for f in cands:
        pack(f, args.inbox)
    sys.stderr.write(f"[pack_case] {len(cands)} 个草稿 → {args.inbox}/draft/\n")


if __name__ == "__main__":
    main()
