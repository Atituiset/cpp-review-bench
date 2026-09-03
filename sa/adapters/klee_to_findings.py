#!/usr/bin/env python3
"""KLEE 符号执行输出 → 归一化 findings（schema/findings.schema.json）。

KLEE 跑完 `klee prog.bc` 后生成输出目录：
  - messages.txt：含 `KLEE: ERROR: <path>:<line>: <msg>` 单行格式
  - testNNN.err：每个错误一个文件，v3.x 为多行格式：
        Error: memory error: out of bound pointer
        File: cases/defect/<cid>/src/xxx.c      （或 klee_src/runtime/... 内部位置）
        Line: 12
        Stack:
            #000000126 in memcpy(...) at klee_src/runtime/Freestanding/memcpy.c:17
            #100000024 in r04_recv(...) at cases/defect/<cid>/src/xxx.c:12

注意两类定位陷阱（v3.2 镜像实测，曾致全部 finding 被静默省略）：
  1. memcpy/memmove 等走 KLEE freestanding runtime 时，错误头部 File/Line
     落在 klee_src/runtime/... 内部，必须沿 Stack 帧回捞首个 cases/ 用户帧；
  2. KLEE 上报路径相对仓根（cases/<track>/<cid>/...），由
     _common.normalize_file 的锚点截断归一。

findings 只来自 KLEE 真实报告的 error（不再接受 golden 反写，也不再做
“无 error 也记一条命中”的自证）。scenario 由错误类型推断；KLEE 的
"out of bound pointer" 不区分读/写，归一为 cwe-787+cwe-125 家族并集。
function 取自命中栈帧（twin 点位区分用）。anchor 按 _common.synth_anchor
合成，合成失败时省略该条并在 stderr 记 warning（不产出无 anchor 的不合规 finding）。

用法：
  klee_to_findings.py <track> <case_id> <klee-out-dir> [--case-dir <dir>] [--out <json>]
"""
import argparse
import json
import re
import sys
from pathlib import Path

from _common import (guess_case_dir, locate_src, make_doc, normalize_file,
                     synth_anchor)

# KLEE 错误类型 → CWE scenario + severity（值已是 schema enum）
KLEE_ERR_MAP = {
    'overflow': ('cwe-787', 'important'),       # 包括 out-of-bounds write
    # KLEE 原文 "out of bound pointer"（无 s）不区分读/写越界，
    # 归一为家族并集，由 eval 的家族相交匹配落到 golden 侧的具体 cwe
    'out of bound': ('cwe-787+cwe-125', 'important'),
    'oob': ('cwe-787+cwe-125', 'important'),
    # 符号化 size 传入 malloc 等被 KLEE 具体化（.model.err），
    # 典型于分配尺寸溢出场景（如 n*size 回绕）
    'concretized symbolic size': ('cwe-190', 'important'),
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


# v3.x .err 多行格式与栈帧
ERR_MSG_RE = re.compile(r'^Error:\s*(.*)$')
ERR_FILE_RE = re.compile(r'^File:\s*(.+)$')
ERR_LINE_RE = re.compile(r'^Line:\s*(\d+)')
STACK_FRAME_RE = re.compile(r'#\d+\s+in\s+(\S+)\(.*\)\s+at\s+(\S+):(\d+)\s*$')
# messages.txt 单行格式
MSG_ERR_RE = re.compile(r'KLEE:\s*ERROR:\s*(.+?):(\d+):\s*(.*)')


def _is_user_path(p: str) -> bool:
    """路径是否落在 case 源码树内（含 cases/<track>/<cid>/ 锚点）。"""
    parts = p.split('/')
    return 'cases' in parts


def parse_err_file(ef: Path):
    """解析一个 .err 文件，返回 {'raw': (file,line,msg), 'site': (file,line,func)}。

    site 为归一后的用户代码位置：头部 File 在 cases/ 内时直接用头部
    （func 取匹配栈帧，通常 #0）；否则沿 Stack 帧自顶向下回捞首个
    cases/ 用户帧（freestanding runtime 的 memcpy/memmove 等场景）。
    捞不到用户帧时 site 退回头部位置（func=None，后续大概率 anchor 合成
    失败被显式省略）。"""
    msg = file = None
    line = 0
    frames = []  # [(func, file, line)] 自顶向下
    for ln in ef.read_text(errors='ignore').splitlines():
        if msg is None:
            m = ERR_MSG_RE.match(ln)
            if m:
                msg = m.group(1).strip()
                continue
        if file is None:
            m = ERR_FILE_RE.match(ln)
            if m:
                file = m.group(1).strip()
                continue
        if not line:
            m = ERR_LINE_RE.match(ln)
            if m:
                line = int(m.group(1))
                continue
        m = STACK_FRAME_RE.search(ln)
        if m:
            frames.append((m.group(1), m.group(2), int(m.group(3))))
    if msg is None or file is None:
        return None
    raw = (file, line, msg)
    if _is_user_path(file):
        func = next((f for f, ff, _ in frames if ff == file), None)
        return {'raw': raw, 'site': (file, line, func)}
    for func, ff, fl in frames:
        if _is_user_path(ff):
            return {'raw': raw, 'site': (ff, fl, func)}
    return {'raw': raw, 'site': (file, line, None)}


def extract_errors(klee_dir: Path):
    """返回 [(file, line, msg, func)] 列表（已按 raw 位置去重）。"""
    errs = []
    seen = set()  # (raw_file, raw_line, msg)
    for ef in sorted(klee_dir.glob('*.err')):
        rec = parse_err_file(ef)
        if not rec:
            continue
        key = rec['raw']
        if key in seen:
            continue
        seen.add(key)
        fpath, line, func = rec['site']
        errs.append((fpath, line, rec['raw'][2], func))
    # messages.txt 兜底（.err 缺失或格式再变时仍有产出）
    msg_file = klee_dir / 'messages.txt'
    if msg_file.is_file():
        for line in msg_file.read_text(errors='ignore').splitlines():
            mm = MSG_ERR_RE.search(line)
            if not mm:
                continue
            key = (mm.group(1).strip(), int(mm.group(2)), mm.group(3).strip())
            if key in seen:
                continue
            seen.add(key)
            errs.append((key[0], key[1], key[2], None))
    return errs


def convert(track, case_id, klee_dir, tool='klee', version=None, out=None,
            case_dir=None):
    kd = Path(klee_dir)
    errs = extract_errors(kd) if kd.is_dir() else []
    findings = []
    for fpath, line, msg, func in errs:
        scen, sev = map_err(msg)
        cdir = case_dir or guess_case_dir(fpath)
        rel = normalize_file(fpath, cdir)
        rel, src = locate_src(cdir, rel)
        anchor = synth_anchor({'message': msg, 'line': line}, src)
        if anchor is None:
            print(f'[warn] {case_id}: 合成不出 anchor（{rel}:{line}），省略该条',
                  file=sys.stderr)
            continue
        f = {'file': rel, 'anchor': anchor, 'message': msg, 'severity': sev}
        if line >= 1:
            f['line'] = int(line)
        if func:
            f['function'] = func
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
