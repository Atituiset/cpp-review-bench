#!/usr/bin/env python3
"""Joern scan.sc 输出 → 归一化 findings（schema/findings.schema.json）。

scan.sc（通用扫描，不依赖 golden）输出 {"findings":[{"file","line","message","scenario"}]}。
本脚本包装为归一化文档：file 归一为相对 case 根的 src/...
（优先 --case-dir，缺省从路径里的 cases/<track>/<cid>/ 锚点推断），
anchor 取源文件对应行内容 strip；行号越界/文件读不到时省略该条并在
stderr 记 warning。

用法：joern_to_findings.py <track> <case_id> <raw-json> [--case-dir <dir>] [--out <json>]
"""
import argparse
import json
import os
import sys
from pathlib import Path


def guess_case_dir(raw_path):
    """从 file 路径里的 cases/<track>/<cid>/ 锚点推断 case 根目录。"""
    parts = os.path.normpath(str(raw_path)).split('/')
    for i, part in enumerate(parts):
        if part == 'cases' and i + 2 < len(parts):
            return '/'.join(parts[:i + 3])
    return None


def normalize_file(raw_path, case_dir=None):
    """归一 file 为相对 case 根的 src/... 形式。"""
    p = os.path.normpath(str(raw_path))
    if case_dir:
        ap = p if os.path.isabs(p) else os.path.join(str(case_dir), p)
        rel = os.path.relpath(ap, str(case_dir))
        if not rel.startswith('..'):
            return rel.replace(os.sep, '/')
    parts = p.split('/')
    for i, part in enumerate(parts):
        if part == 'cases' and i + 3 < len(parts):
            return '/'.join(parts[i + 3:])
    if 'src' in parts:
        return '/'.join(parts[parts.index('src'):])
    return os.path.basename(p)


def line_anchor(case_dir, rel_file, line):
    """读 case 源文件第 line 行，strip 后作 anchor；读不到/越界/空行返回 None。"""
    try:
        lines = (Path(case_dir) / rel_file).read_text(
            encoding='utf-8', errors='replace').splitlines()
    except OSError:
        return None
    if 1 <= line <= len(lines):
        return lines[line - 1].strip() or None
    return None


def make_doc(tool, track, case_id, version, findings):
    doc = {'tool': tool, 'track': track, 'case_id': case_id, 'findings': findings}
    if version and version != 'unknown':
        doc['version'] = version
    return doc


def convert(track, case_id, raw_json, tool='joern', version=None, out=None,
            case_dir=None):
    raw = json.load(open(raw_json, encoding='utf-8', errors='ignore'))
    findings = []
    for r in raw.get('findings', []):
        fpath = r.get('file') or ''
        lineno = int(r.get('line') or 0)
        cdir = case_dir or guess_case_dir(fpath)
        rel = normalize_file(fpath, cdir)
        anchor = line_anchor(cdir, rel, lineno) if cdir else None
        # CPG 的 filename 常是相对 src/ 的短名（如 guti.c），回退补 src/ 前缀再试
        if anchor is None and cdir and not rel.startswith('src/'):
            rel2 = 'src/' + rel
            anchor = line_anchor(cdir, rel2, lineno)
            if anchor is not None:
                rel = rel2
        if anchor is None:
            print(f'[warn] {case_id}: 取不到 anchor（{rel}:{lineno}），省略该条',
                  file=sys.stderr)
            continue
        f = {'file': rel, 'anchor': anchor, 'message': r.get('message', '')}
        if lineno >= 1:
            f['line'] = lineno
        scen = r.get('scenario')
        if scen:
            f['scenario'] = scen
            f['severity'] = 'important'   # 通用危险调用命中，统一记 important
        findings.append(f)
    doc = make_doc(tool, track, case_id, version, findings)
    if out:
        Path(out).parent.mkdir(parents=True, exist_ok=True)
        with open(out, 'w', encoding='utf-8') as fh:
            json.dump(doc, fh, ensure_ascii=False, indent=2)
        print(f'[ok] {case_id}: {len(findings)} joern findings -> {out}')
    else:
        print(json.dumps(doc, ensure_ascii=False, indent=2))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('track'); ap.add_argument('case_id'); ap.add_argument('raw_json')
    ap.add_argument('--tool', default='joern'); ap.add_argument('--version', default=None)
    ap.add_argument('--case-dir', default=None, help='case 根目录（缺省从 file 路径推断）')
    ap.add_argument('--out', default=None)
    args = ap.parse_args()
    convert(args.track, args.case_id, args.raw_json, args.tool, args.version,
            args.out, args.case_dir)


if __name__ == '__main__':
    main()
