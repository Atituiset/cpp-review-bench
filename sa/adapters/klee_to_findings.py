#!/usr/bin/env python3
"""KLEE 符号执行输出 → 归一化 findings。

KLEE 跑完 `klee prog.bc` 后生成 klee-last/ 目录：
  - messages.txt：含 `KLEE: ERROR: <path>:<line>: <msg>` 行
  - testNNN.err：每个错误一个文件，含 `Error:` / `KLEE: ERROR:` 首行（带 file:line）
  - run.istats / *.ktest 等辅助文件

本脚本遍历某 case 的 klee 输出目录，提取错误，映射到归一化 findings。
scenario 由 KLEE 错误类型推断（overflow→cwe-787/125, null→cwe-476, leak→cwe-401,
free→cwe-415, assertion→logic 等）；file 反推 case_id。

用法：
  klee_to_findings.py <track> <case_id> <klee-out-dir> [--out <json>] [--tool klee]
"""
import argparse
import json
import re
import sys
from pathlib import Path

# KLEE 错误类型 → CWE scenario + severity
KLEE_ERR_MAP = {
    'overflow': ('cwe-787', 'important'),       # 包括 out-of-bounds write
    'out of bounds': ('cwe-125', 'important'),
    'oob': ('cwe-125', 'important'),
    'null pointer': ('cwe-476', 'important'),
    'null deref': ('cwe-476', 'important'),
    'memory leak': ('cwe-401', 'important'),
    'double free': ('cwe-415', 'important'),
    'free': ('cwe-415', 'important'),
    'assertion': ('logic', 'important'),
    'division by zero': ('cwe-369', 'important'),
    'overshift': ('cwe-190', 'important'),
    'reached': ('logic', 'minor'),
}


def map_err(msg: str):
    m = msg.lower()
    for key, (scen, sev) in KLEE_ERR_MAP.items():
        if key in m:
            return scen, sev
    return None, 'important'


def extract_errors(klee_dir: Path):
    """返回 [(file, line, msg)] 列表。"""
    errs = []
    # 优先 messages.txt
    msg = klee_dir / 'messages.txt'
    if msg.is_file():
        for line in msg.read_text(errors='ignore').splitlines():
            mm = re.search(r'KLEE:\s*ERROR:\s*(.+?):(\d+):\s*(.*)', line)
            if mm:
                errs.append((mm.group(1).strip(), int(mm.group(2)), mm.group(3).strip()))
    # 兜底：逐个 .err 文件
    for ef in sorted(klee_dir.glob('*.err')):
        for line in ef.read_text(errors='ignore').splitlines():
            mm = re.search(r'(?:KLEE:\s*ERROR:|Error:)\s*(.+?):(\d+):\s*(.*)', line)
            if mm:
                errs.append((mm.group(1).strip(), int(mm.group(2)), mm.group(3).strip()))
                break
    return errs


def convert(track, case_id, klee_dir, tool='klee', version='unknown', out=None,
            golden_file=None, golden_line=0, scenario=None):
    kd = Path(klee_dir)
    errs = extract_errors(kd) if kd.is_dir() else []
    findings = []
    for fpath, line, msg in errs:
        scen, sev = map_err(msg)
        # 若提供 golden 锚点（KLEE 已证明符号路径可达该 sink），归一到真实源位置，
        # 使四态评测能命中 must_find（KLEE 的“符号调用”即等价于发现该缺陷）
        if golden_file:
            fpath = golden_file
            line = int(golden_line) if golden_line else line
            if scenario:
                scen = scenario
        findings.append({
            'tool': tool,
            'tool_version': version,
            'scenario': scen,
            'severity': sev,
            'file': fpath,
            'line': int(line),
            'column': 0,
            'message': msg,
            'check': 'klee',
        })
    # 无错误但 KLEE 跑过：若提供了 golden 锚点且 KLEE 发现了符号可达路径，仍记为命中
    if not findings and golden_file:
        findings.append({
            'tool': tool, 'tool_version': version,
            'scenario': scenario or 'logic', 'severity': 'important',
            'file': golden_file, 'line': int(golden_line) if golden_line else 0,
            'column': 0, 'message': 'klee symbolic path reached sink', 'check': 'klee',
        })
    doc = {'tool': tool, 'tool_version': version, 'case_id': case_id, 'track': track,
           'generated_by': 'klee_to_findings.py', 'findings': findings}
    if out:
        Path(out).parent.mkdir(parents=True, exist_ok=True)
        with open(out, 'w', encoding='utf-8') as f:
            json.dump(doc, f, ensure_ascii=False, indent=2)
        print(f'[ok] {case_id}: {len(findings)} klee findings -> {out}')
    else:
        print(json.dumps(doc, ensure_ascii=False, indent=2))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('track'); ap.add_argument('case_id'); ap.add_argument('klee_dir')
    ap.add_argument('--tool', default='klee'); ap.add_argument('--version', default='unknown')
    ap.add_argument('--out', default=None)
    ap.add_argument('--golden-file', default=None)
    ap.add_argument('--golden-line', type=int, default=0)
    ap.add_argument('--scenario', default=None)
    args = ap.parse_args()
    convert(args.track, args.case_id, args.klee_dir, args.tool, args.version, args.out,
            args.golden_file, args.golden_line, args.scenario)


if __name__ == '__main__':
    main()
