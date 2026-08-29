#!/usr/bin/env python3
"""CodeChecker parse 导出的 JSON → 归一化 findings（按 case 拆分）。

用法：
  codechecker_to_findings.py <cc-json-export> <cases-root> [--out-dir <dir>]

CodeChecker 导出格式（`CodeChecker parse <dir> -e json`）：
  { "reports": [ { "file": {"path":...}, "line", "column",
                   "checker_id": "core.NullDereference", "severity": "HIGH",
                   "message": "..." }, ... ] }

按 file.path 反推 case_id（匹配 cases/<track>/<cid>/src/<name>），
checker_id → CWE scenario 映射（NullDereference→cwe-476 等）。
为每个有 findings 的 case 写出 <cid>.json（归一化）。
"""
import argparse
import json
import os
import re
import sys
from pathlib import Path

# CodeChecker checker_id 前缀 → CWE scenario + severity
CHECKER_MAP = {
    'core.NullDereference': ('cwe-476', 'important'),
    'core.DivideZero': ('cwe-369', 'important'),
    'core.StackAddressEscape': ('cwe-562', 'important'),
    'core.CallAndMessage': ('cwe-688', 'important'),
    'core.MemoryLeak': ('cwe-401', 'important'),
    'core.DoubleFree': ('cwe-415', 'important'),
    'core.UndefinedBinaryOperatorResult': ('logic', 'important'),
    'core.OutOfBounds': ('cwe-125', 'important'),
    'cplusplus.NewDelete': ('cwe-415', 'important'),
    'unix.Malloc': ('cwe-401', 'important'),
    'alpha.core': ('logic', 'important'),
    'clang-analyzer-core.NullDereference': ('cwe-476', 'important'),
    'clang-analyzer-core.DivideZero': ('cwe-369', 'important'),
    'clang-analyzer-cplusplus.NewDelete': ('cwe-415', 'important'),
    'security.insecureAPI': ('cwe-676', 'important'),
    'bugprone': ('logic', 'minor'),
    'clang-analyzer': ('logic', 'minor'),
}


def map_checker(checker_id: str):
    cid = checker_id or ''
    for prefix, (scen, sev) in CHECKER_MAP.items():
        if cid.startswith(prefix) or prefix in cid:
            return scen, sev
    return None, 'info'


def find_case_id(path: str, cases_root: Path):
    p = os.path.normpath(path)
    parts = p.split(os.sep)
    # 找 cases/<track>/<cid>/src/... 结构
    for i, part in enumerate(parts):
        if part == 'cases' and i + 3 < len(parts):
            return parts[i + 2]  # <cid>
    # 退化：找 src 目录的父目录名
    if 'src' in parts:
        idx = parts.index('src')
        if idx >= 1:
            return parts[idx - 1]
    return None


def convert(cc_json, cases_root, out_dir=None):
    cases_root = Path(cases_root)
    if not os.path.isfile(cc_json):
        print(f'[warn] 找不到 {cc_json}，跳过（可能 CodeChecker 无产出）')
        return {}
    with open(cc_json, encoding='utf-8', errors='ignore') as f:
        data = json.load(f)
    reports = data.get('reports', []) if isinstance(data, dict) else []
    by_case = {}
    for r in reports:
        fpath = r.get('file', {})
        if isinstance(fpath, dict):
            fpath = fpath.get('path', '')
        line = r.get('line', 0)
        col = r.get('column', 0)
        checker = r.get('checker_id') or r.get('checker_name') or ''
        msg = r.get('message', '')
        sev_raw = (r.get('severity') or 'info').upper()
        sev = {'HIGH': 'important', 'MEDIUM': 'major', 'LOW': 'minor', 'STYLE': 'info',
               'CRITICAL': 'important', 'INFO': 'info'}.get(sev_raw, 'info')
        scen, map_sev = map_checker(checker)
        if scen is None:
            sev = map_sev
        cid = find_case_id(fpath, cases_root)
        if not cid:
            continue
        by_case.setdefault(cid, []).append({
            'tool': 'codechecker',
            'tool_version': 'unknown',
            'scenario': scen,
            'severity': sev,
            'file': fpath,
            'line': int(line),
            'column': int(col),
            'message': msg,
            'check': checker,
        })
    if out_dir:
        out_dir = Path(out_dir)
        out_dir.mkdir(parents=True, exist_ok=True)
        for cid, finds in by_case.items():
            doc = {'tool': 'codechecker', 'tool_version': 'unknown', 'case_id': cid,
                   'track': 'unknown', 'generated_by': 'codechecker_to_findings.py',
                   'findings': finds}
            with open(out_dir / f'{cid}.json', 'w', encoding='utf-8') as f:
                json.dump(doc, f, ensure_ascii=False, indent=2)
            print(f'[ok] {cid}: {len(finds)} codechecker findings')
    total = sum(len(v) for v in by_case.values())
    print(f'[done] codechecker 共 {total} findings / {len(by_case)} cases')
    return by_case


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('cc_json'); ap.add_argument('cases_root')
    ap.add_argument('--out-dir', default=None)
    args = ap.parse_args()
    convert(args.cc_json, args.cases_root, args.out_dir)


if __name__ == '__main__':
    main()
