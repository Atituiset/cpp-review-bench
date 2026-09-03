#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""sa/adapters 公共逻辑：severity 映射、scenario 家族清洗、anchor 合成、路径归一。

各 *_to_findings.py 均以脚本方式被调用（sa/runners/*.sh 与 ci.yml 里是
`python3 <repo>/sa/adapters/xxx_to_findings.py ...`），脚本所在目录自动进入
sys.path[0]，故 `import _common` 在本地 / CI / docker 各 job 下均可用；
没有任何调用方把 adapters 当包导入，无需相对导入或包结构。

anchor 合成口径与 tools/normalize_evidence.py 完全一致（三级降级）：
  1. message 内嵌 "anchor match: <文本>"（工具上报的原始锚点）
  2. 按 file+line 回读源文件该行内容（strip）
  3. message 内嵌 "dangerous call: <名>" -> 源文件中首个含 "<名>(" 的行
都失败时由各 adapter 显式降级（stderr 记 warning 并省略该条），
不静默产出不合规 findings。
"""
import os
import re
from pathlib import Path

# 工具原始严重度 -> schema enum；不在表内的视为无法判断（省略 severity 字段）
SEVERITY_MAP = {
    'error': 'critical', 'blocker': 'critical', 'critical': 'critical',
    'major': 'critical', 'high': 'critical',
    'warning': 'important', 'medium': 'important',
    'info': 'minor', 'style': 'minor', 'performance': 'minor',
    'portability': 'minor', 'minor': 'minor', 'note': 'minor',
    'remark': 'minor',
}

# scenario 家族 pattern（与 schema/findings.schema.json 的 $defs/scenario 一致）
SCENARIO_RE = re.compile(r'^(cwe-[0-9]+|build|logic)(\+cwe-[0-9]+)*$')

# anchor 合成的 message 线索（与 tools/normalize_evidence.py 同源）
ANCHOR_MATCH_RE = re.compile(r'anchor match:\s*(.+?)(?:\s*\[check:.*)?$')
DANGEROUS_CALL_RE = re.compile(r'dangerous call:\s*(\w+)')


def map_severity(raw):
    """工具原始 severity -> schema 三级枚举；无法判断返回 None（调用方省略字段）。"""
    if not raw:
        return None
    return SEVERITY_MAP.get(str(raw).strip().lower())


def clean_scenario(raw):
    """scenario 家族清洗：不匹配 schema pattern 的原始值（如 codeql 的 cpp/xxx
    规则名）返回 None，与 normalize_evidence.py 一致（原文由调用方并入 message）。"""
    if raw is None:
        return None
    s = str(raw).strip()
    return s if SCENARIO_RE.match(s) else None


def guess_case_dir(raw_path):
    """从 file 路径里的 cases/<track>/<cid>/ 锚点推断 case 根目录。"""
    parts = os.path.normpath(str(raw_path)).split('/')
    for i, part in enumerate(parts):
        if part == 'cases' and i + 2 < len(parts):
            return '/'.join(parts[:i + 3])
    return None


def normalize_file(raw_path, case_dir=None):
    """归一 file 为相对 case 根的 src/... 形式：优先相对 case_dir，
    缺省时从路径里的 cases/<track>/<cid>/ 锚点截断，再退化取 src/ 起。"""
    p = os.path.normpath(str(raw_path))
    if case_dir:
        ap = p if os.path.isabs(p) else os.path.join(str(case_dir), p)
        rel = os.path.relpath(ap, str(case_dir))
        if not rel.startswith('..'):
            return rel.replace(os.sep, '/')
    parts = p.split('/')
    for i, part in enumerate(parts):
        if part == 'cases' and i + 3 < len(parts):
            return '/'.join(parts[i + 3:])   # cases/<track>/<cid>/ 之后
    if 'src' in parts:
        return '/'.join(parts[parts.index('src'):])
    return os.path.basename(p)


def locate_src(case_dir, rel):
    """按 rel 在 case_dir 下定位源文件；bare 文件名（CPG/Infer 常给短名）
    回退补 src/ 前缀再试。返回 (最终 rel, Path|None)。"""
    if not case_dir:
        return rel, None
    src = Path(case_dir) / rel
    if src.is_file():
        return rel, src
    if not rel.startswith('src/'):
        alt = Path(case_dir) / ('src/' + rel)
        if alt.is_file():
            return 'src/' + rel, alt
    return rel, src if src.is_file() else None


def line_anchor(src_file, line):
    """读源文件第 line 行，strip 后作 anchor；读不到/越界/空行返回 None。"""
    try:
        lines = Path(src_file).read_text(
            encoding='utf-8', errors='replace').splitlines()
    except OSError:
        return None
    if 1 <= line <= len(lines):
        return lines[line - 1].strip() or None
    return None


def synth_anchor(f, src):
    """三级 anchor 合成（与 tools/normalize_evidence.py 同一口径）：
    1. message 内嵌 "anchor match: <文本>"；2. 源文件 line 行内容；
    3. message 内嵌 "dangerous call: <名>" -> 源文件首个含 "<名>(" 的行。
    全失败返回 None（调用方显式降级，不得产出无 anchor 的 finding）。"""
    msg = f.get('message') or ''
    m = ANCHOR_MATCH_RE.search(msg)
    if m:
        return m.group(1).strip() or None
    line = f.get('line')
    if src and line:
        anchor = line_anchor(src, line)
        if anchor:
            return anchor
    m = DANGEROUS_CALL_RE.search(msg)
    if m and src:
        call = m.group(1) + '('
        try:
            lines = Path(src).read_text(
                encoding='utf-8', errors='replace').splitlines()
        except OSError:
            return None
        for ln in lines:
            if call in ln and ln.strip():
                return ln.strip()
    return None


def make_doc(tool, track, case_id, version, findings):
    """组归一化 findings 文档；version 取不到（unknown/missing）时省略字段。"""
    doc = {'tool': tool, 'track': track, 'case_id': case_id, 'findings': findings}
    if version and version not in ('unknown', 'missing'):
        doc['version'] = version
    return doc
