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
file 归一为相对 case 根的 src/...；anchor 按 _common.synth_anchor 合成
（源文件行内容回读为主），合成失败时省略该条并在 stderr 记 warning
（不产出无 anchor 的不合规 finding）。

function 提取（供 eval 的 twin 点位按函数区分）：
  1. 报告自带的 issue_context / function 字段（若导出方提供）；
  2. 缺失时按 file+line 回读源文件，花括号深度法回推所在函数
     （CodeChecker 6.28.3 `parse -e json` 导出不含函数上下文——plist 里的
     issue_context 在 JSON 转换时被丢弃，故以源码回推为主路径）；
  都拿不到就省略 function 字段（不写 null）。

为每个有 findings 的 case 写出 <cid>.json（schema/findings.schema.json 形态）。
"""
import argparse
import json
import os
import re
import sys
from pathlib import Path

from _common import (locate_src, make_doc, map_severity, normalize_file,
                     synth_anchor)

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

# 工具原始严重度映射与 anchor 合成等公共逻辑见 _common.py

# 简单 C 函数头识别：返回类型 + 名称 + 形参表，行尾允许换行大括号（K&R 风格）
FUNC_DEF_RE = re.compile(r'^[A-Za-z_][\w\s\*]*?\b(\w+)\s*\([^;{}]*\)\s*(?:\{|$)')
LINE_COMMENT_RE = re.compile(r'//.*')
STRING_RE = re.compile(r'"(?:\\.|[^"\\])*"')


def enclosing_function(src_file, line):
    """按花括号深度粗扫 C 源，返回 line 所在函数的名字；识别不出返回 None。
    启发式：未处理块注释/宏内的花括号，对本仓自然风格用例足够。"""
    try:
        lines = Path(src_file).read_text(
            encoding='utf-8', errors='replace').splitlines()
    except OSError:
        return None
    depth = 0
    current = None   # 当前所在函数体
    pending = None   # 已见函数头、尚未见到 '{'
    for i, raw in enumerate(lines, 1):
        code = STRING_RE.sub('""', LINE_COMMENT_RE.sub('', raw))
        if i == line:
            if depth > 0:
                return current
            m = FUNC_DEF_RE.match(code.strip())
            return m.group(1) if m else None
        if depth == 0:
            m = FUNC_DEF_RE.match(code.strip())
            if m:
                pending = m.group(1)
        o, c = code.count('{'), code.count('}')
        if depth == 0 and o and pending:
            current = pending
            pending = None
        depth += o - c
        if depth <= 0:
            depth = 0
            if c:
                current = None
    return None


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
        rel = normalize_file(fpath)
        rel, src = locate_src(cases_root / track / cid, rel)
        anchor = synth_anchor({'message': msg, 'line': line}, src)
        if anchor is None:
            print(f'[warn] {cid}: 合成不出 anchor（{rel}:{line}），省略该条',
                  file=sys.stderr)
            continue
        f = {'file': rel, 'anchor': anchor, 'message': msg}
        if line >= 1:
            f['line'] = line
        if scen:
            f['scenario'] = scen
        if sev:
            f['severity'] = sev
        # function：优先报告自带字段，缺失时从源码回推所在函数
        func = (r.get('issue_context') or r.get('function') or '').strip()
        if not func and src and line >= 1:
            func = enclosing_function(src, line) or ''
        if func:
            f['function'] = func
        by_case.setdefault(cid, {'track': track, 'finds': []})
        by_case[cid]['finds'].append(f)
    if out_dir:
        out_dir = Path(out_dir)
        out_dir.mkdir(parents=True, exist_ok=True)
        for cid, info in by_case.items():
            doc = make_doc('codechecker', info['track'], cid, version,
                           info['finds'])
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
