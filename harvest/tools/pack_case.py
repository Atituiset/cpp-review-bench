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
    before = (finding.get("evidence", {}) or {}).get("before_slice", "")
    base = os.path.basename(finding.get("file", "snippet.c"))
    # anchor 在 before 切片中的相对行号（使 SARIF 标注指向真实 bug 行）
    anchor = finding.get("anchor")
    rel_line = None
    if anchor and before:
        for i, ln in enumerate(before.splitlines(), 1):
            if anchor.strip() in ln or ln.strip() in anchor:
                rel_line = i
                break
    # src 草稿：before 切片（含真实 bug 行），anchor 行用注释标出便于人审
    src_lines = []
    for i, ln in enumerate(before.splitlines(), 1):
        mark = "  // <<< BUG ANCHOR" if i == rel_line else ""
        src_lines.append(ln + mark)
    with open(os.path.join(d, "src", base), "w") as fh:
        fh.write(f"// AUTO-DRAFT from {finding['evidence'].get('source_repo')} PR #{finding['evidence'].get('pr')}\n")
        fh.write("\n".join(src_lines) + "\n")
    # CMakeLists 占位：单文件 -c 编译目标
    with open(os.path.join(d, "CMakeLists.txt"), "w") as fh:
        fh.write(f"# AUTO-DRAFT; 真实构建片段由 v0.2 补全\nadd_library({cid}_draft STATIC src/{base})\n")
    # golden 草稿：anchor + 相对行号（来自修复 diff 反推，有依据）
    golden = {
        "must_find": [{
            "scenario": finding.get("scenario"),
            "file": f"src/{base}",
            "anchor": anchor,
            "line": rel_line,
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
    # notes 骨架：溯源优先（不猜真值，scenario 标待定）
    ev = finding.get("evidence", {})
    raw_patch = (finding.get("raw", {}) or {}).get("patch", "")
    with open(os.path.join(d, "notes.md"), "w") as fh:
        fh.write(f"# {cid}\n\n")
        fh.write(f"## 来源（采集溯源）\n")
        fh.write(f"- 来源仓: {ev.get('source_repo')}\n")
        fh.write(f"- 源 PR: #{ev.get('pr')} ({ev.get('pr_url')})\n")
        fh.write(f"- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）\n")
        fh.write(f"- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）\n")
        fh.write(f"- 候选初判 scenario: **{finding.get('scenario')}（候选猜测，待 LLM/人审定，非真值）**\n")
        fh.write(f"- 候选初判锚点行: {rel_line}（原始 PR diff 行 {ev.get('anchor_line')}；PR 修复前的代码，待确认是否为 bug）\n\n")
        fh.write(f"## 真实修复前后 diff（PR 改了什么）\n\n")
        if raw_patch:
            fh.write("```diff\n" + raw_patch[:4000] + "\n```\n\n")
        else:
            fh.write("（无 diff）\n\n")
        fh.write(f"## 接受后流程（accept → case）\n")
        fh.write(f"1. 评论 `/case accept {cid}` → 本草稿移入 `cases/defect/{cid}/`（五文件齐备）\n")
        fh.write(f"2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）\n")
        fh.write(f"3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告\n")
        fh.write(f"4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden\n")


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
