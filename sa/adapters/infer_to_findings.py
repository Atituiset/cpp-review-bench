#!/usr/bin/env python3
"""Infer report.json → 归一化 findings（schema/findings.schema.json）。

Infer 输出 infer-out/report.json（JSON），bugs[] 每项含：
  bug_type, qualifier, severity (ERROR/WARNING/INFO),
  line, column, file, procedure, bug_class

口径：
  - 顶层 tool/track/case_id/findings + version（取不到就省略）。
  - scenario 省略（Infer 不带 CWE）。
  - severity 映射到 schema enum（ERROR→critical，WARNING→important，INFO→minor）。
  - file 归一为相对 case 根的 src/...（优先 --case-dir，缺省从路径里的
    cases/<track>/<cid>/ 锚点推断）；anchor 按 _common.synth_anchor 合成
    （源文件行内容回读为主，bare 文件名回退补 src/ 前缀）。
    合成失败时省略该条并在 stderr 记 warning（不产出无 anchor 的不合规 finding）。
"""
import argparse
import glob as _glob
import json
import sys
from pathlib import Path

from _common import (guess_case_dir, locate_src, make_doc, map_severity,
                     normalize_file, synth_anchor)


def convert(track, case_id, report_json, tool, version, case_dir=None):
    p = Path(report_json)
    files = [str(p)] if p.is_file() else sorted(
        _glob.glob(str(p / '**' / 'report.json'), recursive=True))
    bugs = []
    for rp in files:
        data = json.load(open(rp, encoding='utf-8'))
        bugs += data.get('bugs', []) if isinstance(data, dict) else data

    findings = []
    for b in bugs:
        fpath = b.get('file', '')
        # case 根：优先 --case-dir，缺省从该条 file 路径推断
        cdir = case_dir or guess_case_dir(fpath)
        rel = normalize_file(fpath, cdir)
        lineno = int(b.get('line') or 0)
        msg = f"{b.get('bug_type','')}: {b.get('qualifier','')}".strip(': ')
        rel, src = locate_src(cdir, rel)
        anchor = synth_anchor({'message': msg, 'line': lineno}, src)
        if anchor is None:
            print(f'[warn] {case_id}: 合成不出 anchor（{rel}:{lineno}），省略该条',
                  file=sys.stderr)
            continue
        f = {'file': rel, 'anchor': anchor, 'message': msg}
        if lineno >= 1:
            f['line'] = lineno
        sev = map_severity(b.get('severity'))
        if sev:
            f['severity'] = sev
        proc = (b.get('procedure') or '').strip()
        if proc:
            f['function'] = proc
        findings.append(f)
    return make_doc(tool, track, case_id, version, findings)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('track'); ap.add_argument('case_id'); ap.add_argument('report_json')
    ap.add_argument('--tool', default='infer'); ap.add_argument('--version', default=None)
    ap.add_argument('--case-dir', default=None, help='case 根目录（缺省从 file 路径推断）')
    ap.add_argument('--out', required=False, default=None, help='输出归一化 findings 路径')
    args = ap.parse_args()

    out = convert(args.track, args.case_id, args.report_json, args.tool,
                  args.version, args.case_dir)
    if args.out:
        Path(args.out).parent.mkdir(parents=True, exist_ok=True)
        with open(args.out, 'w', encoding='utf-8') as f:
            json.dump(out, f, ensure_ascii=False, indent=2)
        print(f'[ok] {args.case_id}: {len(out["findings"])} findings -> {args.out}')
    else:
        print(json.dumps(out, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
