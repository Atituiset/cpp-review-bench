#!/usr/bin/env python3
"""Joern scan.sc 输出 → 归一化 findings。

scan.sc 输出 {"findings":[{"file","line","column","message","scenario"}]}。
本脚本包装为归一化文档（补 tool/case_id/track），按 case 写出一个 <cid>.json。
用法：joern_to_findings.py <track> <case_id> <raw-json> [--out <json>]
"""
import argparse
import json
import sys
from pathlib import Path


def convert(track, case_id, raw_json, tool='joern', version='unknown', out=None):
    raw = json.load(open(raw_json, encoding='utf-8', errors='ignore'))
    findings = []
    for f in raw.get('findings', []):
        findings.append({
            'tool': tool,
            'tool_version': version,
            'scenario': f.get('scenario'),
            'severity': 'important' if f.get('scenario') else 'info',
            'file': f.get('file'),
            'line': int(f.get('line', 0)),
            'column': int(f.get('column', 0)),
            'message': f.get('message', ''),
            'check': 'joern-cpg',
        })
    doc = {'tool': tool, 'tool_version': version, 'case_id': case_id, 'track': track,
           'generated_by': 'joern_to_findings.py', 'findings': findings}
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
    ap.add_argument('--tool', default='joern'); ap.add_argument('--version', default='unknown')
    ap.add_argument('--out', default=None)
    args = ap.parse_args()
    convert(args.track, args.case_id, args.raw_json, args.tool, args.version, args.out)


if __name__ == '__main__':
    main()
