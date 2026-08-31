#!/usr/bin/env python3
"""CppCheck 适配：单 case 源目录 -> 归一化 findings。

用法:
  python3 tools/cppcheck_to_findings.py <track> <case_id> <case_src_dir> \
        [--cppcheck cppcheck] [--tool cppcheck] [--version 2.13]

说明:
  - 调用 cppcheck 对整个 src_dir 做过程内/过程间分析，输出 XML 到 stderr。
  - cppcheck 的 error id 不直接带 CWE 编号；本适配器把 scenario 置 null，
    anchor 取源文件对应行源码（去空白），eval.py 的 L1 按 file+anchor 匹配。
  - 仅收集落在 case_src_dir 内的 location（忽略系统头/外部文件）。
"""
import argparse
import json
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


def norm(s: str) -> str:
    return re.sub(r"\s+", "", s)


# 工具原始严重度 -> schema enum；不在表内的视为无法判断（省略 severity 字段）
SEVERITY_MAP = {
    "error": "critical", "blocker": "critical", "critical": "critical",
    "major": "critical", "high": "critical",
    "warning": "important", "medium": "important",
    "info": "minor", "style": "minor", "performance": "minor",
    "portability": "minor", "minor": "minor", "note": "minor",
}


def map_severity(raw: str) -> str | None:
    if not raw:
        return None
    return SEVERITY_MAP.get(str(raw).strip().lower())


def line_anchor(src_file: Path, line: int) -> str | None:
    try:
        lines = src_file.read_text(encoding="utf-8", errors="replace").splitlines()
    except FileNotFoundError:
        return None
    if 1 <= line <= len(lines):
        return lines[line - 1].strip()
    return None


def make_doc(tool: str, track: str, case_id: str, version: str | None,
             findings: list) -> dict:
    doc = {"tool": tool, "track": track, "case_id": case_id, "findings": findings}
    if version and version not in ("unknown", "missing"):
        doc["version"] = version
    return doc


def convert(track: str, case_id: str, src_dir: Path,
            cppcheck_bin: str, tool: str, version: str,
            include_dirs: list[str] | None = None) -> dict:
    sources = sorted(str(p) for p in src_dir.glob("*.c"))
    if not sources:
        return make_doc(tool, track, case_id, version, [])
    cmd = [cppcheck_bin, "--enable=warning,style,performance,portability,information",
           "--inconclusive", "--xml", "--xml-version=2"]
    if include_dirs:
        for d in include_dirs:
            cmd += ["-I", d]
    cmd += sources
    proc = subprocess.run(cmd, capture_output=True, text=True)
    raw = proc.stderr
    findings = []
    try:
        root = ET.fromstring(raw)
    except ET.ParseError:
        # cppcheck 有时在 XML 前后吐非 XML 行，截取 <results>...</results>
        m = re.search(r"<results>.*</results>", raw, re.S)
        if not m:
            sys.stderr.write(f"[warn] cppcheck XML 解析失败 for {case_id}\n")
            return make_doc(tool, track, case_id, version, [])
        root = ET.fromstring(m.group(0))
    for err in root.iter("error"):
        sev = err.get("severity", "")
        if sev in ("information",):  # "include not found" 等噪声，不计入 bench findings
            continue
        for loc in err.findall("location"):
            abs_file = loc.get("file")
            line = loc.get("line")
            if not abs_file or not line:
                continue
            p = Path(abs_file).resolve()
            try:
                rel = p.relative_to(src_dir.resolve())
            except ValueError:
                continue  # 系统头/外部文件，忽略
            src_file = src_dir / rel
            anchor = line_anchor(src_file, int(line))
            if anchor is None:
                continue
            # scenario 未知（cppcheck 无 CWE 标签）故省略
            f = {
                "file": str(rel).replace("\\", "/"),
                "line": int(line),
                "anchor": anchor,
                "message": err.get("msg", ""),
            }
            sev = map_severity(err.get("severity", ""))
            if sev:
                f["severity"] = sev
            findings.append(f)
    return make_doc(tool, track, case_id, version, findings)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("track")
    ap.add_argument("case_id")
    ap.add_argument("src_dir")
    ap.add_argument("--cppcheck", default="cppcheck")
    ap.add_argument("--tool", default="cppcheck")
    ap.add_argument("--version", default="unknown")
    ap.add_argument("--include-dir", action="append", default=[],
                    help="传给 cppcheck 的 -I 系统头目录（可多次）")
    args = ap.parse_args()

    if shutil.which(args.cppcheck) is None:
        sys.stderr.write(f"[warn] 未找到 {args.cppcheck}，跳过 {args.case_id}\n")
        json.dump(make_doc(args.tool, args.track, args.case_id, "missing", []),
                  sys.stdout, ensure_ascii=False, indent=2)
        sys.stdout.write("\n")
        return

    out = convert(args.track, args.case_id, Path(args.src_dir),
                  args.cppcheck, args.tool, args.version,
                  include_dirs=args.include_dir or None)
    json.dump(out, sys.stdout, ensure_ascii=False, indent=2)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
