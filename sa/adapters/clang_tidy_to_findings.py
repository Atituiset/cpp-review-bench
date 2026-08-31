#!/usr/bin/env python3
"""clang-tidy 输出 → 归一化 findings（schema/findings.schema.json）。

clang-tidy 纯文本诊断格式（解析 stderr）：
    <file>:<line>:<col>: warning: <msg> [<check-name>]
    <file>:<line>:<col>: error:   <msg> [<check-name>]

口径：
  - 顶层 tool/track/case_id/findings + version（取不到就省略）。
  - scenario 省略（clang-tidy 不直接给 CWE）。
  - severity 由 level 映射到 schema enum（error→critical，warning→important）。
  - file 归一为相对 case 根的 src/...；anchor 取该源文件对应行内容 strip。
    行号越界/文件读不到时省略该条并在 stderr 记 warning（不静默吞）。
"""
import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

DIAG_RE = re.compile(
    r'^(?P<file>[^:]+):(?P<line>\d+):(?P<col>\d+):'
    r'\s*(?P<level>warning|error|note|remark):\s*(?P<msg>.*?)'
    r'(?:\s*\[(?P<check>[^\]]+)\])?\s*$'
)

# 工具原始严重度 -> schema enum；不在表内的视为无法判断（省略 severity 字段）
SEVERITY_MAP = {
    'error': 'critical', 'blocker': 'critical', 'critical': 'critical',
    'major': 'critical', 'high': 'critical',
    'warning': 'important', 'medium': 'important',
    'info': 'minor', 'style': 'minor', 'performance': 'minor',
    'portability': 'minor', 'minor': 'minor', 'note': 'minor',
    'remark': 'minor',
}


def map_severity(raw):
    if not raw:
        return None
    return SEVERITY_MAP.get(str(raw).strip().lower())


def normalize_file(raw_path, case_dir=None):
    """归一 file 为相对 case 根的 src/... 形式：优先相对 case_dir，
    缺省时从路径里的 cases/<track>/<cid>/ 锚点截断，再退化取 src/ 起。"""
    p = os.path.normpath(str(raw_path))
    if case_dir:
        ap = p if os.path.isabs(p) else os.path.join(str(case_dir), p)
        rel = os.path.relpath(ap, str(case_dir))
        if not rel.startswith('..'):
            return rel.replace(os.sep, '/')
    parts = p.split('/')
    for i, part in enumerate(parts):
        if part == 'cases' and i + 3 < len(parts):
            return '/'.join(parts[i + 3:])   # cases/<track>/<cid>/ 之后
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


def convert(track, case_id, src_dir, clang_tidy_bin, tool, version,
            build_dir=None, case_dir=None):
    src_dir = Path(src_dir)
    case_dir = Path(case_dir) if case_dir else src_dir.parent
    sources = sorted(str(p) for p in src_dir.glob('*.c')) + sorted(str(p) for p in src_dir.glob('*.cpp'))
    if not sources:
        print(f'[warn] {case_id}: no source file', file=sys.stderr)
        return None

    cmd = [clang_tidy_bin, '-checks=bugprone-*,clang-analyzer-*', f'-p={build_dir or src_dir.parent.parent}']
    cmd += sources
    proc = subprocess.run(cmd, capture_output=True, text=True)
    findings = []
    for line in proc.stderr.splitlines():
        m = DIAG_RE.match(line.strip())
        if not m:
            continue
        if m.group('level') not in ('error', 'warning'):
            continue   # 只收 error/warning，note 不计入四态
        rel = normalize_file(m.group('file'), case_dir)
        lineno = int(m.group('line'))
        anchor = line_anchor(case_dir, rel, lineno)
        if anchor is None:
            print(f'[warn] {case_id}: 取不到 anchor（{rel}:{lineno}），省略该条',
                  file=sys.stderr)
            continue
        f = {'file': rel, 'anchor': anchor, 'line': lineno,
             'message': m.group('msg').strip()}
        sev = map_severity(m.group('level'))
        if sev:
            f['severity'] = sev
        findings.append(f)
    return make_doc(tool, track, case_id, version, findings)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('track')
    ap.add_argument('case_id')
    ap.add_argument('src_dir')
    ap.add_argument('--clang-tidy', default='clang-tidy')
    ap.add_argument('--build-dir', default=None, help='compile_commands.json 所在目录（默认 src_dir 的上级）')
    ap.add_argument('--case-dir', default=None, help='case 根目录（默认 src_dir 的父目录）')
    ap.add_argument('--out', required=True)
    ap.add_argument('--tool', default='clang-tidy')
    ap.add_argument('--version', default=None)
    args = ap.parse_args()

    out = convert(args.track, args.case_id, args.src_dir, args.clang_tidy,
                  args.tool, args.version, args.build_dir, args.case_dir)
    if out is None:
        out = make_doc(args.tool, args.track, args.case_id, args.version, [])
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    with open(args.out, 'w', encoding='utf-8') as f:
        json.dump(out, f, ensure_ascii=False, indent=2)
    print(f'[ok] {args.case_id}: {len(out["findings"])} findings -> {args.out}')


if __name__ == '__main__':
    main()
