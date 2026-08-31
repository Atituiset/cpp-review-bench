#!/usr/bin/env python3
"""通用 SARIF → 归一化 findings（schema/findings.schema.json，按 case 拆分）。

支持 CodeQL 等产出 SARIF 2.1 的工具。从 runs[].results[] 提取：
  ruleId（如 cpp/cwe-476 / cwe-190）、level（error/warning/note）、
  message.text、locations[].physicalLocation.{artifactLocation.uri, region.startLine}。

口径：
  - scenario 由 ruleId 解析：含 cwe-NNN 则映射为 cwe-NNN；命中 CODEQL_CWE_MAP
    则用表值；否则省略（原始 ruleId 不符合 scenario 的 schema 模式，不能照抄）。
  - severity 由 level 映射到 schema enum（error→critical，warning→important，
    note→minor），无法判断则省略。
  - file 归一为相对 case 根的 src/...（从 cases/<track>/<cid>/ 锚点截断）；
    anchor 取源文件对应行内容 strip（源文件按 --repo-root 定位），
    行号越界/文件读不到时省略该条并在 stderr 记 warning。
  - version 优先取 SARIF 里 tool.driver.version，其次 --version 参数。
"""
import argparse
import json
import re
import sys
from pathlib import Path

# 工具原始严重度 -> schema enum；不在表内的视为无法判断（省略 severity 字段）
SEVERITY_MAP = {
    'error': 'critical', 'blocker': 'critical', 'critical': 'critical',
    'major': 'critical', 'high': 'critical',
    'warning': 'important', 'medium': 'important',
    'info': 'minor', 'style': 'minor', 'performance': 'minor',
    'portability': 'minor', 'minor': 'minor', 'note': 'minor',
}

CWE_RE = re.compile(r'cwe-(\d+)', re.IGNORECASE)

# CodeQL ruleId（cpp/xxx）-> CWE 场景映射（使 eval 的 scenario 家族匹配可用）
CODEQL_CWE_MAP = {
    'cpp/integer-overflow': 'cwe-190',
    'cpp/integer-multiplication-wrap': 'cwe-190',
    'cpp/overflow': 'cwe-125',          # 保守：buffer over-read；越界写同族归 cwe-787 时另判
    'cpp/buffer-overflow': 'cwe-787',
    'cpp/pointer-dereference': 'cwe-476',
    'cpp/use-after-free': 'cwe-416',
    'cpp/double-free': 'cwe-415',
    'cpp/leak': 'cwe-401',
    'cpp/comparison-with-wrong-type': 'cwe-190',
}


def map_severity(raw):
    if not raw:
        return None
    return SEVERITY_MAP.get(str(raw).strip().lower())


def scenario_from_rule(rule_id: str):
    if not rule_id:
        return None
    m = CWE_RE.search(rule_id)
    if m:
        return f"cwe-{m.group(1)}"
    if rule_id.lower() in CODEQL_CWE_MAP:
        return CODEQL_CWE_MAP[rule_id.lower()]
    # 未映射的 ruleId 不符合 scenario 的 schema 模式（cwe-N|build|logic），省略
    return None


def line_anchor(src_file: Path, line: int):
    """读源文件第 line 行，strip 后作 anchor；读不到/越界/空行返回 None。"""
    try:
        lines = src_file.read_text(encoding='utf-8', errors='replace').splitlines()
    except OSError:
        return None
    if 1 <= line <= len(lines):
        return lines[line - 1].strip() or None
    return None


def convert(sarif_path, out_dir, tool='codeql', version=None, repo_root='.'):
    data = json.load(open(sarif_path, encoding='utf-8'))
    runs = data.get('runs', [])
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    repo_root = Path(repo_root)

    # 版本：优先 SARIF 内的 tool.driver.version
    if not version or version == 'unknown':
        for run in runs:
            v = run.get('tool', {}).get('driver', {}).get('version')
            if v:
                version = v
                break

    # 按 case 归并：用 artifact uri 反推 case_id（cases/<track>/<cid>/src/<file>）
    by_case = {}
    for run in runs:
        for res in run.get('results', []):
            rule_id = res.get('ruleId', '')
            level = res.get('level', 'warning')
            msg = res.get('message', {}).get('text', '')
            locs = res.get('locations', [])
            if not locs:
                continue
            phys = locs[0].get('physicalLocation', {})
            art = phys.get('artifactLocation', {}).get('uri', '')
            line = int(phys.get('region', {}).get('startLine') or 0)
            # 反推 case_id / track，并把 file 截断为相对 case 根的 src/...
            m = re.search(r'cases/([^/]+)/([^/]+)/(src/.*)$', art)
            if not m:
                print(f'[warn] 无法从 {art} 反推 case，省略该条', file=sys.stderr)
                continue
            track, cid, rel = m.group(1), m.group(2), m.group(3)
            if track not in ('contract', 'defect', 'calibration'):
                print(f'[warn] {art}: track={track} 非法，省略该条', file=sys.stderr)
                continue
            anchor = line_anchor(repo_root / 'cases' / track / cid / rel, line)
            if anchor is None:
                print(f'[warn] {cid}: 取不到 anchor（{rel}:{line}），省略该条',
                      file=sys.stderr)
                continue
            f = {'file': rel, 'anchor': anchor, 'message': msg}
            if line >= 1:
                f['line'] = line
            scen = scenario_from_rule(rule_id)
            if scen:
                f['scenario'] = scen
            sev = map_severity(level)
            if sev:
                f['severity'] = sev
            by_case.setdefault(cid, {'track': track, 'finds': []})
            by_case[cid]['finds'].append(f)

    for cid, info in by_case.items():
        doc = {'tool': tool, 'track': info['track'], 'case_id': cid,
               'findings': info['finds']}
        if version and version != 'unknown':
            doc['version'] = version
        with open(out_dir / f"{cid}.json", 'w', encoding='utf-8') as fh:
            json.dump(doc, fh, ensure_ascii=False, indent=2)
        print(f'[ok] {cid}: {len(info["finds"])} findings -> {out_dir}/{cid}.json')
    print(f'[done] 共 {len(by_case)} 个 case 的 SARIF findings -> {out_dir}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('sarif')
    ap.add_argument('out_dir')
    ap.add_argument('--tool', default='codeql')
    ap.add_argument('--version', default=None)
    ap.add_argument('--repo-root', default='.',
                    help='仓库根（定位 cases/... 源文件取 anchor，默认当前目录）')
    args = ap.parse_args()
    convert(args.sarif, args.out_dir, args.tool, args.version, args.repo_root)


if __name__ == '__main__':
    main()
