#!/usr/bin/env python3
"""CodeChecker parse 导出的 JSON → 归一化 findings（按 case 拆分）。

用法：
  codechecker_to_findings.py <cc-json-export> <cases-root> [--out-dir <dir>] [--version <ver>]

CodeChecker 导出格式（`CodeChecker parse <dir> -e json`）：
  { "reports": [ { "file": {"path":...}, "line", "column",
                   "checker_id": "core.NullDereference", "severity": "HIGH",
                   "message": "..." }, ... ] }

按 file.path 反推 case_id（匹配 cases/<track>/<cid>/src/<name>），
checker_id → CWE scenario 映射（NullDereference→cwe-476 等）。
file 归一为相对 case 根的 src/...；anchor 取 cases-root 下源文件对应行内容
strip，行号越界/文件读不到时省略该条并在 stderr 记 warning。
为每个有 findings 的 case 写出 <cid>.json（schema/findings.schema.json 形态）。
"""
import argparse
import json
import os
import sys
from pathlib import Path

# CodeChecker checker_id 前缀 → CWE scenario + severity（值已是 schema enum）
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


def map_checker(checker_id: str):
    cid = checker_id or ''
    for prefix, (scen, sev) in CHECKER_MAP.items():
        if cid.startswith(prefix) or prefix in cid:
            return scen, sev
    return None, None


def find_case_id(path: str, cases_root: Path):
    p = os.path.normpath(path)
    parts = p.split(os.sep)
    # 找 cases/<track>/<cid>/src/... 结构
    for i, part in enumerate(parts):
        if part == 'cases' and i + 3 < len(parts):
            return parts[i + 2], parts[i + 1]  # (<cid>, <track>)
    # 退化：找 src 目录的父目录名
    if 'src' in parts:
        idx = parts.index('src')
        if idx >= 2 and parts[idx - 2] == 'cases':
            return parts[idx - 1], parts[idx - 2]
    return None, None


def rel_path(path: str):
    """把绝对/长路径收敛为相对 case 根的 src/... 形式（与 golden 的 file 同口径）。"""
    p = os.path.normpath(path)
    parts = p.split(os.sep)
    for i, part in enumerate(parts):
        if part == 'cases' and i + 3 < len(parts):
            return '/'.join(parts[i + 3:])  # cases/<track>/<cid>/ 之后
    if 'src' in parts:
        idx = parts.index('src')
        return '/'.join(parts[idx:])  # src/xxx.c
    return os.path.basename(p)


def line_anchor(src_file: Path, line: int):
    """读源文件第 line 行，strip 后作 anchor；读不到/越界/空行返回 None。"""
    try:
        lines = src_file.read_text(encoding='utf-8', errors='replace').splitlines()
    except OSError:
        return None
    if 1 <= line <= len(lines):
        return lines[line - 1].strip() or None
    return None


def convert(cc_json, cases_root, out_dir=None, version=None):
    cases_root = Path(cases_root)
    if not os.path.isfile(cc_json):
        print(f'[warn] {cc_json} not found, skip (CodeChecker may have no output)',
              file=sys.stderr)
        return {}
    with open(cc_json, encoding='utf-8', errors='ignore') as f:
        data = json.load(f)
    reports = data.get('reports', []) if isinstance(data, dict) else []
    by_case = {}
    for r in reports:
        fpath = r.get('file', {})
        if isinstance(fpath, dict):
            fpath = fpath.get('path', '')
        line = int(r.get('line') or 0)
        checker = r.get('checker_id') or r.get('checker_name') or ''
        msg = r.get('message', '')
        # severity：优先 checker 映射的定级，否则映射 CodeChecker 原始 severity
        scen, sev = map_checker(checker)
        if sev is None:
            sev = map_severity(r.get('severity'))
        cid, track = find_case_id(fpath, cases_root)
        if not cid:
            continue
        # 若 track 仍未解析，从 cases_root 反查 golden.json
        if not track:
            for t in ('contract', 'defect', 'calibration'):
                if (cases_root / t / cid / 'golden.json').exists():
                    track = t
                    break
        if not track:
            print(f'[warn] {cid}: track 无法解析（{fpath}），跳过该 case',
                  file=sys.stderr)
            continue
        rel = rel_path(fpath)
        anchor = line_anchor(cases_root / track / cid / rel, line)
        if anchor is None:
            print(f'[warn] {cid}: 取不到 anchor（{rel}:{line}），省略该条',
                  file=sys.stderr)
            continue
        f = {'file': rel, 'anchor': anchor, 'message': msg}
        if line >= 1:
            f['line'] = line
        if scen:
            f['scenario'] = scen
        if sev:
            f['severity'] = sev
        by_case.setdefault(cid, {'track': track, 'finds': []})
        by_case[cid]['finds'].append(f)
    if out_dir:
        out_dir = Path(out_dir)
        out_dir.mkdir(parents=True, exist_ok=True)
        for cid, info in by_case.items():
            doc = {'tool': 'codechecker', 'track': info['track'], 'case_id': cid,
                   'findings': info['finds']}
            if version and version != 'unknown':
                doc['version'] = version
            with open(out_dir / f'{cid}.json', 'w', encoding='utf-8') as f:
                json.dump(doc, f, ensure_ascii=False, indent=2)
            print(f'[ok] {cid}: {len(info["finds"])} codechecker findings')
    total = sum(len(v['finds']) for v in by_case.values())
    print(f'[done] codechecker 共 {total} findings / {len(by_case)} cases')
    return by_case


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('cc_json'); ap.add_argument('cases_root')
    ap.add_argument('--out-dir', default=None)
    ap.add_argument('--version', default=None, help='CodeChecker 版本（取不到则省略）')
    args = ap.parse_args()
    convert(args.cc_json, args.cases_root, args.out_dir, args.version)


if __name__ == '__main__':
    main()
