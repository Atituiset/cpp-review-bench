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
import re
import subprocess
import sys
from pathlib import Path

from _common import line_anchor, make_doc, map_severity, normalize_file

DIAG_RE = re.compile(
    r'^(?P<file>[^:]+):(?P<line>\d+):(?P<col>\d+):'
    r'\s*(?P<level>warning|error|note|remark):\s*(?P<msg>.*?)'
    r'(?:\s*\[(?P<check>[^\]]+)\])?\s*$'
)


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
        anchor = line_anchor(Path(case_dir) / rel, lineno)
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
