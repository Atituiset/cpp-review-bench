#!/usr/bin/env python3
"""通用 SARIF → 归一化 findings。

支持 CodeQL 等产出 SARIF 1.0/2.1 的工具。从 runs[].results[] 提取：
  ruleId（如 cpp/cwe-476 / cwe-190）、level（error/warning/note）、
  message.text、locations[].physicalLocation.{artifactLocation.uri, region.startLine/startColumn}。

scenario 由 ruleId 解析：若含 cwe-NNN 则映射为 cwe-NNN；否则 null。
severity 由 level 映射：error→error, warning→warning, note→info。
"""
import argparse
import json
import re
import sys
from pathlib import Path

LEVEL_SEV = {'error': 'error', 'warning': 'warning', 'note': 'info', 'none': 'info'}
CWE_RE = re.compile(r'cwe-(\d+)', re.IGNORECASE)


def scenario_from_rule(rule_id: str):
    if not rule_id:
        return None
    m = CWE_RE.search(rule_id)
    if m:
        return f"cwe-{m.group(1)}"
    return None


def convert(sarif_path, out_dir, tool='codeql', version='unknown'):
    data = json.load(open(sarif_path, encoding='utf-8'))
    runs = data.get('runs', [])
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # 收集 rule 元数据（id -> name）
    rule_names = {}
    for run in runs:
        for r in run.get('tool', {}).get('driver', {}).get('rules', []):
            rule_names[r.get('id')] = r.get('name', '')

    # 按 case 归并：用 artifact uri 反推 case_id（cases/<track>/<cid>/src/<file>）
    by_case = {}
    for run in runs:
        results = run.get('results', [])
        for res in results:
            rule_id = res.get('ruleId', '')
            level = res.get('level', 'warning')
            msg = res.get('message', {}).get('text', '')
            locs = res.get('locations', [])
            if not locs:
                continue
            phys = locs[0].get('physicalLocation', {})
            art = phys.get('artifactLocation', {}).get('uri', '')
            region = phys.get('region', {})
            line = region.get('startLine', 0)
            col = region.get('startColumn', 0)
            # 反推 case_id
            m = re.search(r'cases/([^/]+)/([^/]+)/src/', art)
            track = m.group(1) if m else 'unknown'
            cid = m.group(2) if m else 'unknown'
            scen = scenario_from_rule(rule_id)
            by_case.setdefault(cid, {
                'tool': tool, 'tool_version': version, 'case_id': cid, 'track': track,
                'generated_by': 'sarif_to_findings.py', 'findings': []
            })
            by_case[cid]['findings'].append({
                'tool': tool,
                'tool_version': version,
                'scenario': scen,
                'severity': LEVEL_SEV.get(level, 'info'),
                'file': art,
                'line': int(line),
                'column': int(col),
                'message': msg,
                'check': rule_id,
            })

    for cid, doc in by_case.items():
        with open(out_dir / f"{cid}.json", 'w', encoding='utf-8') as f:
            json.dump(doc, f, ensure_ascii=False, indent=2)
        print(f'[ok] {cid}: {len(doc["findings"])} findings -> {out_dir}/{cid}.json')
    print(f'[done] 共 {len(by_case)} 个 case 的 SARIF findings -> {out_dir}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('sarif')
    ap.add_argument('out_dir')
    ap.add_argument('--tool', default='codeql')
    ap.add_argument('--version', default='unknown')
    args = ap.parse_args()
    convert(args.sarif, args.out_dir, args.tool, args.version)


if __name__ == '__main__':
    main()
