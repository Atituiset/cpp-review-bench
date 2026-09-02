#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""findings_to_sarif.py —— 归一化 findings → SARIF 2.1.0（PR 态可视化层）。

canonical 工件（findings.json）不变，SARIF 仅作展示/交换层：
  - github/codeql-action/upload-sarif 上传 → PR 代码行内联标注 + code-scanning check
  - VSCode SARIF Viewer / reviewdog 均可直接消费

形状参照 agent-reviewer@mvp 的 artifact-to-sarif.sh：
  - ruleId = scenario（cwe-XXX）
  - level 由 severity 映射（error/warning/note）
  - region.snippet 嵌源码行
  - message.markdown 嵌判断理由 / 证据链
  - partialFingerprints.findingIndex 去重锚

用法:
  findings_to_sarif.py <findings.json> [out.sarif]
  findings_to_sarif.py --dir <cases_dir> [out.sarif]   # 合并目录下所有 findings.json（多 case）
  findings_to_sarif.py --candidate <draft_case_dir> [out.sarif]  # 单候选：读 golden + 合成 findings
"""
import argparse
import json
import os
import sys
from pathlib import Path

LEVEL = {"error": "error", "critical": "error", "important": "error",
         "warning": "warning", "medium": "warning", "note": "note",
         "info": "note", "low": "note", "minor": "note"}

SEV_NAME = {"error": "严重", "warning": "警告", "note": "提示"}


def read_text_snippet(uri: str, line: int, span: int = 3) -> str:
    """从被审源码读 region.snippet（SARIF 渲染用）。"""
    if not uri or not line:
        return ""
    p = Path(uri)
    if not p.is_file():
        return ""
    try:
        lines = p.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return ""
    start = max(0, line - 1)
    return "\n".join(lines[start:start + span])[:400]


def build_rules(findings):
    """从 findings 收集 scenario → rule 元数据。"""
    rules = {}
    for f in findings:
        scen = f.get("scenario") or "unknown"
        if scen in rules:
            continue
        rules[scen] = {
            "id": scen,
            "name": scen,
            "shortDescription": {"text": f"scenario {scen}"},
            "fullDescription": {"text": f"静态分析场景 {scen}"},
            "properties": {"cwe": scen if scen.startswith("cwe-") else "null"},
        }
    return list(rules.values())


def convert_doc(doc: dict, src_root: str = "", uri_prefix: str = "") -> dict:
    """把单个归一化 findings doc 投影为 SARIF run 的 results + rules。"""
    findings = doc.get("findings", [])
    tool = doc.get("tool", "unknown")
    case_id = doc.get("case_id", "unknown")
    track = doc.get("track", "defect")
    # 未显式给 --uri-prefix 时：bench 用例的 findings.file 是相对 case 根的路径
    # （如 src/recv.c），SARIF 标注需映射到仓内真实路径 cases/<track>/<case_id>/
    uri_prefix = uri_prefix or f"cases/{track}/{case_id}"
    rules = build_rules(findings)
    results = []
    for i, f in enumerate(findings):
        raw_uri = f.get("file") or ""
        uri = (uri_prefix.rstrip("/") + "/" + raw_uri) if (uri_prefix and raw_uri) else raw_uri
        line = int(f.get("line") or f.get("region", {}).get("startLine") or 0)
        col = int(f.get("column") or f.get("region", {}).get("startColumn") or 1)
        scen = f.get("scenario") or "unknown"
        sev = f.get("severity", "warning")
        level = LEVEL.get(sev, "warning")
        # 源码 snippet：优先用 findings 自带 anchor，否则读文件
        snippet = f.get("anchor") or ""
        if not snippet and src_root:
            snippet = read_text_snippet(os.path.join(src_root, uri), line)
        # message.markdown 嵌理由 + 证据链
        md = f.get("message", "") or f.get("summary", "")
        flow = f.get("flow") or []
        flow_md = ""
        if flow:
            flow_md = "\n\n**证据链**：\n" + "\n".join(
                f"- `{s.get('file', uri)}:{s.get('line', '?')}` {s.get('message', '')}"
                for s in flow)
        reasoning = f.get("reasoning", "")
        reason_md = f"\n\n**判断理由**：{reasoning}" if reasoning else ""
        message = {
            "text": f"[{case_id}] {scen}: {md}",
            "markdown": (f"**{SEV_NAME.get(level, '警告')}** `{scen}` — {md}"
                         + (f"\n\n```\n{snippet}\n```" if snippet else "")
                         + reason_md + flow_md),
        }
        loc = {
            "physicalLocation": {
                "artifactLocation": {"uri": uri},
                "region": {
                    "startLine": line if line else 1,
                    "startColumn": col if col else 1,
                    "endLine": line if line else 1,
                    "snippet": {"text": snippet} if snippet else {},
                },
            }
        }
        results.append({
            "ruleId": scen,
            "level": level,
            "message": message,
            "locations": [loc],
            "partialFingerprints": {
                "findingIndex": f"{case_id}#{i}",
                "primaryLocationLineHash": f"{uri}:{line}",
            },
            "properties": {
                "tool": tool,
                "case_id": case_id,
                "track": track,
                "severity": sev,
                "confidence": f.get("confidence", "n/a"),
            },
        })
    return {"rules": rules, "results": results, "tool": tool, "case_id": case_id}


def merge_to_sarif(docs, src_root: str = "", uri_prefix: str = "") -> dict:
    all_rules = {}
    all_results = []
    tool_names = []
    for doc in docs:
        conv = convert_doc(doc, src_root, uri_prefix)
        for r in conv["rules"]:
            all_rules[r["id"]] = r
        all_results.extend(conv["results"])
        tool_names.append(conv["tool"])
    sarif = {
        "$schema": "https://json.schemastore.org/sarif-2.1.0.json",
        "version": "2.1.0",
        "runs": [{
            "tool": {
                "driver": {
                    "name": "cpp-review-bench",
                    "informationUri": "https://github.com/<org>/cpp-review-bench",
                    "rules": list(all_rules.values()),
                }
            },
            "results": all_results,
        }],
    }
    return sarif


def load_findings_doc(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def main():
    ap = argparse.ArgumentParser(description="归一化 findings → SARIF 2.1.0")
    ap.add_argument("findings", nargs="?", help="findings.json 或 candidates 目录")
    ap.add_argument("out", nargs="?", default=None, help="输出 .sarif")
    ap.add_argument("--dir", help="合并目录下所有 findings.json")
    ap.add_argument("--candidate", help="单候选目录（读 golden.json 合成 findings）")
    ap.add_argument("--src-root", default="", help="源码根（读 region.snippet）")
    ap.add_argument("--uri-prefix", default="",
                    help="findings 的 file URI 前缀（如 harvest/inbox/draft/<cid>），使 SARIF 标注映射到 PR 内的新文件路径")
    args = ap.parse_args()

    docs = []
    if args.candidate:
        cdir = Path(args.candidate)
        golden = json.loads((cdir / "golden.json").read_text(encoding="utf-8"))
        track = golden.get("track", "defect")
        cid = cdir.name
        # 合成 findings：每个 must_find 作为一条（基于猜测 scenario）
        findings = []
        for g in golden.get("must_find", []):
            findings.append({
                "scenario": g.get("scenario"),
                "severity": g.get("severity", "warning"),
                "file": g.get("file", ""),
                "line": g.get("line", 1),
                "anchor": g.get("anchor", ""),
                "message": g.get("rationale", "候选初判 bug 点（非真值，待 LLM/人审定）"),
                "reasoning": "harvest 候选：scenario 为启发式初判，待正式仓手动 LLM 评审定真值",
            })
        docs.append({"tool": "harvest-draft", "track": track, "case_id": cid,
                     "findings": findings})
    elif args.dir:
        d = Path(args.dir)
        for p in sorted(d.glob("*.json")):
            docs.append(load_findings_doc(p))
    elif args.findings:
        docs.append(load_findings_doc(Path(args.findings)))
    else:
        ap.error("需指定 findings / --dir / --candidate")

    sarif = merge_to_sarif(docs, args.src_root, args.uri_prefix)
    out = args.out or (Path(args.findings).with_suffix(".sarif") if args.findings
                       else "review.sarif")
    Path(out).write_text(json.dumps(sarif, ensure_ascii=False, indent=2),
                         encoding="utf-8")
    print(f"[ok] SARIF -> {out}（{len(sarif['runs'][0]['results'])} results, "
          f"{len(sarif['runs'][0]['tool']['driver']['rules'])} rules）")


if __name__ == "__main__":
    main()
