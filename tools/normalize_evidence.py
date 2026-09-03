#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""一次性归一化：把 reports/evidence/ 归档 findings 修到 schema/findings.schema.json 合规。

归一化规则（保留原始数据语义）：
- 顶层 tool_version -> version（schema 既有字段，同义改名）；generated_by 移除
  （产出方信息已由 tool 字段承载）
- finding 级多余字段 tool/tool_version/column 移除；check（规则 id）并入 message
  （"[check: X]" 后缀，信息不丢）
- severity 映射到三级枚举：error->critical, warning->important, style/info->minor
- function: null -> 省略字段（schema 只允许 string）
- scenario 不匹配家族 pattern（如 codeql 的 cpp/xxx 规则名）-> null
  （schema 允许 null，家族匹配退化为不强制；原文经 check/message 保留）
- 缺 anchor（schema 必填）-> 按优先级合成：
  1. message 内嵌 "anchor match: <文本>"（工具上报的原始锚点）
  2. 源文件 line 行内容（file 依次尝试 用例相对 / 用例 src 下 / 仓根相对）
  3. message 内嵌 "dangerous call: <名>" -> 源文件中首个含 "<名>(" 的行
  都失败则报错并保留原文件不改，由人工处理

summary 文件（*summary*.json）是 eval.py run 的汇总产物、不是 findings 文档，跳过。

用法：python3 tools/normalize_evidence.py [--dry-run]
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EVIDENCE = ROOT / "reports/evidence"

SEVERITY_MAP = {"error": "critical", "warning": "important",
                "style": "minor", "info": "minor"}
SCENARIO_RE = re.compile(r"^(cwe-[0-9]+|build|logic)(\+cwe-[0-9]+)*$")
ANCHOR_MATCH_RE = re.compile(r"anchor match:\s*(.+?)(?:\s*\[check:.*)?$")
DANGEROUS_CALL_RE = re.compile(r"dangerous call:\s*(\w+)")

DROP_TOP = ("generated_by",)
DROP_FINDING = ("tool", "tool_version", "column")


def norm(s: str) -> str:
    return re.sub(r"\s+", "", s or "")


def find_source(track: str, cid: str, fpath: str) -> Path | None:
    """定位 finding.file 对应的源文件：用例相对 -> 用例 src 下 -> 仓根相对。"""
    case_dir = ROOT / "cases" / track / cid
    for cand in (case_dir / fpath,
                 case_dir / "src" / Path(fpath).name,
                 ROOT / fpath):
        if cand.is_file():
            return cand
    return None


def synth_anchor(f: dict, src: Path | None) -> str | None:
    msg = f.get("message") or ""
    m = ANCHOR_MATCH_RE.search(msg)
    if m:
        return m.group(1).strip() or None
    line = f.get("line")
    if src and line:
        lines = src.read_text(encoding="utf-8", errors="replace").splitlines()
        if 1 <= line <= len(lines) and lines[line - 1].strip():
            return lines[line - 1].strip()
    m = DANGEROUS_CALL_RE.search(msg)
    if m and src:
        call = m.group(1) + "("
        for ln in src.read_text(encoding="utf-8", errors="replace").splitlines():
            if call in ln and ln.strip():
                return ln.strip()
    return None


def normalize_doc(doc: dict, path: Path, errors: list) -> dict:
    out = dict(doc)
    # 顶层：tool_version -> version；generated_by 移除
    if "tool_version" in out:
        out.setdefault("version", out.pop("tool_version"))
    for k in DROP_TOP:
        out.pop(k, None)
    track, cid = out.get("track"), out.get("case_id")
    findings = []
    for i, f in enumerate(out.get("findings", [])):
        f = dict(f)
        check = f.pop("check", None)
        for k in DROP_FINDING:
            f.pop(k, None)
        # function: null -> 省略
        if f.get("function") is None:
            f.pop("function", None)
        # severity 映射到三级枚举
        sev = f.get("severity")
        if sev in SEVERITY_MAP:
            f["severity"] = SEVERITY_MAP[sev]
        # scenario 不匹配家族 pattern -> null（原文随后随 check/message 保留）
        sc = f.get("scenario")
        orig_sc = None
        if sc is not None and not SCENARIO_RE.match(str(sc)):
            orig_sc = str(sc)
            f["scenario"] = None
        # 缺 anchor -> 合成（须在 message 追加 check/scenario 后缀之前，避免污染锚点）
        if not f.get("anchor"):
            src = find_source(track, cid, f.get("file", "")) if track and cid else None
            anchor = synth_anchor(f, src)
            if not anchor:
                errors.append(f"{path}: findings[{i}] 无法合成 anchor "
                              f"(file={f.get('file')} line={f.get('line')})")
            else:
                f["anchor"] = anchor
        # check（规则 id）与非法 scenario 原文并入 message，信息不丢
        if check:
            msg = f.get("message") or ""
            if f"[check: {check}]" not in msg:
                f["message"] = (msg + " " if msg else "") + f"[check: {check}]"
        if orig_sc and orig_sc != check:
            msg = f.get("message") or ""
            if orig_sc not in msg:
                f["message"] = (msg + " " if msg else "") + f"[scenario: {orig_sc}]"
        findings.append(f)
    out["findings"] = findings
    return out


def main():
    dry = "--dry-run" in sys.argv
    errors = []
    changed = skipped = 0
    for p in sorted(EVIDENCE.glob("**/*.json")):
        if "summary" in p.name:
            skipped += 1
            continue  # eval.py run 的汇总产物，非 findings 文档
        doc = json.loads(p.read_text(encoding="utf-8"))
        if not (isinstance(doc, dict) and "findings" in doc
                and "track" in doc and "case_id" in doc):
            errors.append(f"{p}: 结构不似 findings 文档，未处理")
            continue
        out = normalize_doc(doc, p, errors)
        if out != doc:
            changed += 1
            if not dry:
                p.write_text(json.dumps(out, ensure_ascii=False, indent=2) + "\n",
                             encoding="utf-8")
    print(f"{'[dry-run] ' if dry else ''}重写 {changed} 个文件，跳过 summary {skipped} 个")
    for e in errors:
        print(f"[ERROR] {e}", file=sys.stderr)
    sys.exit(1 if errors else 0)


if __name__ == "__main__":
    main()
