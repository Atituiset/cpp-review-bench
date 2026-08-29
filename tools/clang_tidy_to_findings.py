#!/usr/bin/env python3
"""clang-tidy 输出 → 归一化 findings。

clang-tidy 每行诊断格式（带 --export-fixes 是 YAML，这里解析纯文本 stdout）：
    <file>:<line>:<col>: warning: <msg> [<check-name>]
    <file>:<line>:<col>: error:   <msg> [<check-name>]

scenario 统一为 null（clang-tidy 不直接给 CWE），severity 由 level 映射
（error→error, warning→warning, 其余→info）。
"""
import argparse
import json
import re
import sys
from pathlib import Path

DIAG_RE = re.compile(
    r'^(?P<file>[^:]+):(?P<line>\d+):(?P<col>\d+):'
    r'\s*(?P<level>warning|error|note|remark):\s*(?P<msg>.*?)'
    r'(?:\s*\[(?P<check>[^\]]+)\])?\s*$'
)

LEVEL_SEV = {'error': 'error', 'warning': 'warning', 'note': 'info', 'remark': 'info'}


def convert(track, case_id, src_dir, clang_tidy_bin, tool, version, build_dir=None):
    src_dir = Path(src_dir)
    sources = sorted(str(p) for p in src_dir.glob('*.c')) + sorted(str(p) for p in src_dir.glob('*.cpp'))
    if not sources:
        print(f'[warn] {case_id}: 无源文件', file=sys.stderr)
        return None

    cmd = [clang_tidy_bin, f'-p={build_dir or src_dir.parent.parent}']
    cmd += sources
    import subprocess
    proc = subprocess.run(cmd, capture_output=True, text=True)
    findings = []
    for line in proc.stderr.splitlines():
        m = DIAG_RE.match(line.strip())
        if not m:
            continue
        if m.group('level') not in ('error', 'warning'):
            continue   # 只收 error/warning，note 不计入四态
        f = m.group('file')
        try:
            rel = str(Path(f).relative_to(src_dir.parent.parent))
        except ValueError:
            rel = f
        findings.append({
            'tool': tool,
            'tool_version': version,
            'scenario': None,
            'severity': LEVEL_SEV[m.group('level')],
            'file': rel,
            'line': int(m.group('line')),
            'column': int(m.group('col')),
            'message': m.group('msg').strip(),
            'check': m.group('check') or '',
        })
    out = {
        'tool': tool,
        'tool_version': version,
        'case_id': case_id,
        'track': track,
        'generated_by': 'clang_tidy_to_findings.py',
        'findings': findings,
    }
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('track')
    ap.add_argument('case_id')
    ap.add_argument('src_dir')
    ap.add_argument('--clang-tidy', default='clang-tidy')
    ap.add_argument('--build-dir', default=None, help='compile_commands.json 所在目录（默认 src_dir 的上级）')
    ap.add_argument('--out', required=True)
    ap.add_argument('--tool', default='clang-tidy')
    ap.add_argument('--version', default='unknown')
    args = ap.parse_args()

    out = convert(args.track, args.case_id, args.src_dir, args.clang_tidy,
                  args.tool, args.version, args.build_dir)
    if out is None:
        out = {'tool': args.tool, 'tool_version': args.version, 'case_id': args.case_id,
               'track': args.track, 'generated_by': 'clang_tidy_to_findings.py', 'findings': []}
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    with open(args.out, 'w', encoding='utf-8') as f:
        json.dump(out, f, ensure_ascii=False, indent=2)
    print(f'[ok] {args.case_id}: {len(out["findings"])} findings -> {args.out}')


if __name__ == '__main__':
    main()
