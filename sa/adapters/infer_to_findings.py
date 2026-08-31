#!/usr/bin/env python3
"""Infer report.json → 归一化 findings（schema/findings.schema.json）。

Infer 输出 infer-out/report.json（JSON），bugs[] 每项含：
  bug_type, qualifier, severity (ERROR/WARNING/INFO),
  line, column, file, procedure, bug_class

口径：
  - 顶层 tool/track/case_id/findings + version（取不到就省略）。
  - scenario 省略（Infer 不带 CWE）。
  - severity 映射到 schema enum（ERROR→critical，WARNING→important，INFO→minor）。
  - file 归一为相对 case 根的 src/...（优先 --case-dir，缺省从路径里的
    cases/<track>/<cid>/ 锚点推断）；anchor 取源文件对应行内容 strip。
    行号越界/文件读不到时省略该条并在 stderr 记 warning。
"""
import argparse
import glob as _glob
import json
import os
import sys
from pathlib import Path

# 工具原始严重度 -> schema enum；不在表内的视为无法判断（省略 severity 字段）
SEVERITY_MAP = {
    'error': 'critical', 'blocker': 'critical', 'critical': 'critical',
    'major': 'critical', 'high': 'critical',
    'warning': 'important', 'medium': 'important',
    'info': 'minor', 'style': 'minor', 'performance': 'minor',
    'portability': 'minor', 'minor': 'minor', 'note': 'minor',
}


def map_severity(raw):
    if not raw:
        return None
    return SEVERITY_MAP.get(str(raw).strip().lower())


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


def convert(track, case_id, report_json, tool, version, case_dir=None):
    p = Path(report_json)
    files = [str(p)] if p.is_file() else sorted(
        _glob.glob(str(p / '**' / 'report.json'), recursive=True))
    bugs = []
    for rp in files:
        data = json.load(open(rp, encoding='utf-8'))
        bugs += data.get('bugs', []) if isinstance(data, dict) else data

    findings = []
    for b in bugs:
        fpath = b.get('file', '')
        # case 根：优先 --case-dir，缺省从该条 file 路径推断
        cdir = case_dir or guess_case_dir(fpath)
        rel = normalize_file(fpath, cdir)
        lineno = int(b.get('line') or 0)
        anchor = line_anchor(cdir, rel, lineno) if cdir else None
        if anchor is None:
            print(f'[warn] {case_id}: 取不到 anchor（{rel}:{lineno}），省略该条',
                  file=sys.stderr)
            continue
        f = {'file': rel, 'anchor': anchor,
             'message': f"{b.get('bug_type','')}: {b.get('qualifier','')}".strip(': ')}
        if lineno >= 1:
            f['line'] = lineno
        sev = map_severity(b.get('severity'))
        if sev:
            f['severity'] = sev
        proc = (b.get('procedure') or '').strip()
        if proc:
            f['function'] = proc
        findings.append(f)
    return make_doc(tool, track, case_id, version, findings)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('track'); ap.add_argument('case_id'); ap.add_argument('report_json')
    ap.add_argument('--tool', default='infer'); ap.add_argument('--version', default=None)
    ap.add_argument('--case-dir', default=None, help='case 根目录（缺省从 file 路径推断）')
    ap.add_argument('--out', required=False, default=None, help='输出归一化 findings 路径')
    args = ap.parse_args()

    out = convert(args.track, args.case_id, args.report_json, args.tool,
                  args.version, args.case_dir)
    if args.out:
        Path(args.out).parent.mkdir(parents=True, exist_ok=True)
        with open(args.out, 'w', encoding='utf-8') as f:
            json.dump(out, f, ensure_ascii=False, indent=2)
        print(f'[ok] {args.case_id}: {len(out["findings"])} findings -> {args.out}')
    else:
        print(json.dumps(out, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
