#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""reports/evidence/ 归档 findings 的 schema 校验门禁（CI build-and-eval 调用）。

对 reports/evidence/**/*.json 逐文件做 schema/findings.schema.json 校验。
*summary*.json 是 eval.py run 的汇总产物（cases/summary 结构），不是 findings 文档，
明确排除；其余文件必须 100% 合规，否则非零退出。
"""
import json
import sys
from pathlib import Path

import jsonschema

ROOT = Path(__file__).resolve().parent.parent
schema = json.loads((ROOT / "schema/findings.schema.json").read_text(encoding="utf-8"))
validator = jsonschema.Draft202012Validator(schema)

failures = skipped = checked = 0
for p in sorted((ROOT / "reports/evidence").glob("**/*.json")):
    if "summary" in p.name:
        skipped += 1
        continue
    checked += 1
    try:
        doc = json.loads(p.read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        print(f"[FAIL] {p.relative_to(ROOT)}: JSON 解析失败: {e}")
        failures += 1
        continue
    errs = list(validator.iter_errors(doc))
    if errs:
        failures += 1
        print(f"[FAIL] {p.relative_to(ROOT)}: {len(errs)} 处不合规")
        for e in errs[:5]:
            loc = "/".join(str(x) for x in e.absolute_path) or "<root>"
            print(f"  - {loc}: {e.message}")
if failures:
    print(f"evidence 校验: {checked} 个 findings 文件中 {failures} 个不合规")
else:
    print(f"evidence 校验: {checked} 个 findings 文件全部合规"
          f"（跳过 summary 产物 {skipped} 个）")
sys.exit(1 if failures else 0)
