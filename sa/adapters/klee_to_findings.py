#!/usr/bin/env python3
"""KLEE 符号执行输出 → 归一化 findings（schema/findings.schema.json）。

KLEE 跑完 `klee prog.bc` 后生成 klee-last/ 目录：
  - messages.txt：含 `KLEE: ERROR: <path>:<line>: <msg>` 行
  - testNNN.err：每个错误一个文件，含 `Error:` / `KLEE: ERROR:` 首行（带 file:line）

findings 只来自 KLEE 真实报告的 error（不再接受 golden 反写，也不再做
“无 error 也记一条命中”的自证）。scenario 由错误类型推断
（overflow→cwe-787/125, null→cwe-476, leak→cwe-401, free→cwe-415 等）。
file 归一为相对 case 根的 src/...（优先 --case-dir，缺省从路径里的
cases/<track>/<cid>/ 锚点推断）；anchor 取源文件对应行内容 strip，
行号越界/文件读不到时省略该条并在 stderr 记 warning。

用法：
  klee_to_findings.py <track> <case_id> <klee-out-dir> [--case-dir <dir>] [--out <json>]
"""
import argparse
import json
import os
import re
import sys
from pathlib import Path

# KLEE 错误类型 → CWE scenario + severity（值已是 schema enum）
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
}


def map_err(msg: str):
    m = msg.lower()
    for key, (scen, sev) in KLEE_ERR_MAP.items():
        if key in m:
            return scen, sev
    return None, 'important'


def guess_case_dir(raw_path):
    """从 file 路径里的 cases/<track>/<cid>/ 锚点推断 case 根目录。"""
    parts = os.path.normpath(str(raw_path)).split('/')
    for i, part in enumerate(parts):
        if part == 'cases' and i + 2 < len(parts):
            return '/'.join(parts[:i + 3])
    return None


def normalize_file(raw_path, case_dir=None):
    """归一 file 为相对 case 根的 src/... 形式。"""
    p = os.path.normpath(str(raw_path))
    if case_dir:
        ap = p if os.path.isabs(p) else os.path.join(str(case_dir), p)
        rel = os.path.relpath(ap, str(case_dir))
        if not rel.startswith('..'):
            return rel.replace(os.sep, '/')
    parts = p.split('/')
    for i, part in enumerate(parts):
        if part == 'cases' and i + 3 < len(parts):
            return '/'.join(parts[i + 3:])
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


def convert(track, case_id, klee_dir, tool='klee', version=None, out=None,
            case_dir=None):
    kd = Path(klee_dir)
    errs = extract_errors(kd) if kd.is_dir() else []
    findings = []
    for fpath, line, msg in errs:
        scen, sev = map_err(msg)
        cdir = case_dir or guess_case_dir(fpath)
        rel = normalize_file(fpath, cdir)
        anchor = line_anchor(cdir, rel, line) if cdir else None
        if anchor is None:
            print(f'[warn] {case_id}: 取不到 anchor（{rel}:{line}），省略该条',
                  file=sys.stderr)
            continue
        f = {'file': rel, 'anchor': anchor, 'message': msg, 'severity': sev}
        if line >= 1:
            f['line'] = int(line)
        if scen:
            f['scenario'] = scen
        findings.append(f)
    doc = make_doc(tool, track, case_id, version, findings)
    if out:
        Path(out).parent.mkdir(parents=True, exist_ok=True)
        with open(out, 'w', encoding='utf-8') as fh:
            json.dump(doc, fh, ensure_ascii=False, indent=2)
        print(f'[ok] {case_id}: {len(findings)} klee findings -> {out}')
    else:
        print(json.dumps(doc, ensure_ascii=False, indent=2))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('track'); ap.add_argument('case_id'); ap.add_argument('klee_dir')
    ap.add_argument('--tool', default='klee'); ap.add_argument('--version', default=None)
    ap.add_argument('--case-dir', default=None, help='case 根目录（缺省从 file 路径推断）')
    ap.add_argument('--out', default=None)
    args = ap.parse_args()
    convert(args.track, args.case_id, args.klee_dir, args.tool, args.version,
            args.out, args.case_dir)


if __name__ == '__main__':
    main()
