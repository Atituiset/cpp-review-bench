#!/usr/bin/env python3
"""Clang Static Analyzer 适配：scan-build 风格的 plist 目录 -> 归一化 findings。

用法:
  python3 tools/csa_to_findings.py <track> <case_id> <case_src_dir> <plist_dir> [--tool csa] [--version 21.1.8]

说明:
  - CSA 默认输出不含 CWE 编号，只有 checker 名（如 "Out of bound memory access"）。
    工具侧不知道 scenario 时，本适配器把 scenario 置 null，anchor 取源文件对应行源码
    （去空白），eval.py 的 L1 仍可仅按 file+anchor 做存在性匹配。
  - plist 的 location.file 是绝对路径，映射到 case_src_dir 下的相对路径（src/...）。
"""
import argparse
import json
import plistlib
import re
import sys
from pathlib import Path


def norm(s: str) -> str:
    return re.sub(r"\s+", "", s)


def rel_path_for(abs_file: str, src_dir: Path) -> str | None:
    p = Path(abs_file).resolve()
    try:
        rel = p.relative_to(src_dir.resolve())
        return str(rel).replace("\\", "/")
    except ValueError:
        # 绝对路径可能落在 src_dir 之外（如系统头），忽略
        return None


def line_anchor(src_file: Path, line: int) -> str | None:
    try:
        lines = src_file.read_text(encoding="utf-8", errors="replace").splitlines()
    except FileNotFoundError:
        return None
    if 1 <= line <= len(lines):
        return lines[line - 1].strip()
    return None


def convert(track: str, case_id: str, src_dir: Path, plist_dir: Path,
            tool: str, version: str) -> dict:
    findings = []
    for pl in sorted(plist_dir.glob("*.plist")):
        with pl.open("rb") as fh:
            data = plistlib.load(fh)
        diags = data.get("diagnostics", [])
        for d in diags:
            loc = d.get("location", {})
            abs_file = loc.get("file")
            line = loc.get("line")
            if not abs_file or not line:
                continue
            rel = rel_path_for(abs_file, src_dir)
            if rel is None:
                continue
            src_file = src_dir / rel
            anchor = line_anchor(src_file, int(line))
            if anchor is None:
                continue
            # scenario 未知（CSA 无 CWE 标签）故省略；severity 无法从 plist 判定故省略
            f = {
                "file": rel,
                "line": int(line),
                "anchor": anchor,
                "message": d.get("description", ""),
            }
            func = (d.get("issue_context") or "").strip()
            if func:
                f["function"] = func
            findings.append(f)
    doc = {"tool": tool, "track": track, "case_id": case_id,
           "findings": findings}
    if version and version != "unknown":
        doc["version"] = version
    return doc


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("track")
    ap.add_argument("case_id")
    ap.add_argument("src_dir")
    ap.add_argument("plist_dir")
    ap.add_argument("--tool", default="csa")
    ap.add_argument("--version", default="unknown")
    args = ap.parse_args()

    out = convert(args.track, args.case_id, Path(args.src_dir),
                  Path(args.plist_dir), args.tool, args.version)
    json.dump(out, sys.stdout, ensure_ascii=False, indent=2)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
