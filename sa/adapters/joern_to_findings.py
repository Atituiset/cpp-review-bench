#!/usr/bin/env python3
"""Joern scan.sc 输出 → 归一化 findings（schema/findings.schema.json）。

scan.sc（通用扫描，不依赖 golden）输出
{"findings":[{"file","line","message","scenario","function"}]}
（function 为 CPG 上调用点所在 method 名，映射到 findings.function，
供 eval 的 twin 点位按函数区分；旧版 scan.sc 输出无此字段时自动省略）。
本脚本包装为归一化文档：file 归一为相对 case 根的 src/...
（优先 --case-dir，缺省从路径里的 cases/<track>/<cid>/ 锚点推断），
anchor 按 _common.synth_anchor 三级合成（message 内嵌锚点文本 →
源文件行内容回读 → "dangerous call: X" 首个调用行）；都失败时省略该条
并在 stderr 记 warning（不产出无 anchor 的不合规 finding）。
scenario 不符合家族 pattern 时置空并把原文并入 message
（与 tools/normalize_evidence.py 同口径）。

用法：joern_to_findings.py <track> <case_id> <raw-json> [--case-dir <dir>] [--out <json>]
"""
import argparse
import json
import sys
from pathlib import Path

from _common import (clean_scenario, guess_case_dir, locate_src, make_doc,
                     normalize_file, synth_anchor)


def convert(track, case_id, raw_json, tool='joern', version=None, out=None,
            case_dir=None):
    raw = json.load(open(raw_json, encoding='utf-8', errors='ignore'))
    findings = []
    for r in raw.get('findings', []):
        fpath = r.get('file') or ''
        lineno = int(r.get('line') or 0)
        msg = r.get('message', '')
        cdir = case_dir or guess_case_dir(fpath)
        rel = normalize_file(fpath, cdir)
        # CPG 的 filename 常是相对 src/ 的短名（如 guti.c），locate_src 回退补 src/ 前缀
        rel, src = locate_src(cdir, rel)
        anchor = synth_anchor({'message': msg, 'line': lineno}, src)
        if anchor is None:
            print(f'[warn] {case_id}: 合成不出 anchor（{rel}:{lineno}），省略该条',
                  file=sys.stderr)
            continue
        scen_raw = r.get('scenario')
        scen = clean_scenario(scen_raw)
        if scen_raw and scen is None:
            # 家族 pattern 之外的 scenario 原文并入 message，信息不丢
            msg = (msg + ' ' if msg else '') + f'[scenario: {scen_raw}]'
        f = {'file': rel, 'anchor': anchor, 'message': msg}
        if lineno >= 1:
            f['line'] = lineno
        if scen_raw:
            f['severity'] = 'important'   # 通用危险调用命中，统一记 important
        if scen:
            f['scenario'] = scen
        func = (r.get('function') or '').strip()
        # CPG 伪节点名（如 <global>/<module>）不是真实函数，不写
        if func and not func.startswith('<'):
            f['function'] = func
        findings.append(f)
    doc = make_doc(tool, track, case_id, version, findings)
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
    ap.add_argument('--tool', default='joern'); ap.add_argument('--version', default=None)
    ap.add_argument('--case-dir', default=None, help='case 根目录（缺省从 file 路径推断）')
    ap.add_argument('--out', default=None)
    args = ap.parse_args()
    convert(args.track, args.case_id, args.raw_json, args.tool, args.version,
            args.out, args.case_dir)


if __name__ == '__main__':
    main()
