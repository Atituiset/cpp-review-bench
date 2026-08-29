#!/usr/bin/env python3
"""Infer report.json → 归一化 findings。

Infer 输出 infer-out/report.json（JSON），bugs[] 每项含：
  bug_type, qualifier, severity (ERROR/WARNING/INFO),
  line, column, file, procedure, bug_class
scenario 统一为 null（Infer 不带 CWE），severity 映射 ERROR→error, WARNING→warning。
"""
import argparse
import json
import sys
from pathlib import Path

SEV = {'ERROR': 'error', 'WARNING': 'warning', 'INFO': 'info', 'LIKE': 'info'}


def convert(track, case_id, report_json, tool, version):
    p = Path(report_json)
    if not p.exists():
        return {'tool': tool, 'tool_version': version, 'case_id': case_id,
                'track': track, 'generated_by': 'infer_to_findings.py', 'findings': []}
    data = json.load(open(p, encoding='utf-8'))
    bugs = data.get('bugs', []) if isinstance(data, dict) else data
    findings = []
    for b in bugs:
        fpath = b.get('file', '')
        try:
            rel = str(Path(fpath).relative_to(Path.cwd().parent))
        except ValueError:
            rel = fpath
        findings.append({
            'tool': tool,
            'tool_version': version,
            'scenario': None,
            'severity': SEV.get(b.get('severity', 'INFO'), 'info'),
            'file': rel,
            'line': int(b.get('line', 0)),
            'column': int(b.get('column', 0)),
            'message': f"{b.get('bug_type','')}: {b.get('qualifier','')}".strip(': '),
            'check': b.get('bug_type', ''),
        })
    return {'tool': tool, 'tool_version': version, 'case_id': case_id, 'track': track,
            'generated_by': 'infer_to_findings.py', 'findings': findings}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('track'); ap.add_argument('case_id'); ap.add_argument('report_json')
    ap.add_argument('--tool', default='infer'); ap.add_argument('--version', default='unknown')
    args = ap.parse_args()
    out = convert(args.track, args.case_id, args.report_json, args.tool, args.version)
    Path(args.out).parent.mkdir(parents=True, exist_ok=True) if hasattr(args, 'out') else None
    print(json.dumps(out, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
