#!/usr/bin/env python3
"""前 3 例交审前自检：golden 过 schema + anchor/file 真实存在。"""
import json, re, sys
from pathlib import Path

import jsonschema

ROOT = Path(__file__).resolve().parent.parent
schema = json.loads((ROOT / "schema/golden.schema.json").read_text(encoding="utf-8"))

def norm(s: str) -> str:
    return re.sub(r"\s+", "", s)

failures = 0
for gj in sorted(ROOT.glob("cases/*/*/golden.json")):
    case_dir = gj.parent
    golden = json.loads(gj.read_text(encoding="utf-8"))
    cid = case_dir.name
    try:
        jsonschema.validate(golden, schema)
    except jsonschema.ValidationError as e:
        print(f"[FAIL] {cid}: schema: {e.message}")
        failures += 1
        continue
    if golden["id"] != cid:
        print(f"[FAIL] {cid}: golden.id={golden['id']} 与目录名不一致")
        failures += 1
    if golden["track"] != case_dir.parent.name:
        print(f"[FAIL] {cid}: track={golden['track']} 与所在轨目录不一致")
        failures += 1
    entries = golden["expected"]["must_find"] + golden["expected"]["must_not_find"]
    for ent in entries:
        f, anchor = ent.get("file"), ent.get("anchor")
        if f is None:
            continue
        target = case_dir / f
        if not target.is_file():
            print(f"[FAIL] {cid}: file 不存在: {f}")
            failures += 1
            continue
        if anchor and norm(anchor) not in norm(target.read_text(encoding="utf-8")):
            print(f"[FAIL] {cid}: anchor 在 {f} 中不存在: {anchor!r}")
            failures += 1
    # contract 轨必须随车带 contract.yaml，且契约名一致
    if golden["track"] == "contract":
        cy = case_dir / "contract.yaml"
        cname = golden.get("context", {}).get("contract")
        if not cy.is_file():
            print(f"[FAIL] {cid}: contract 轨缺 contract.yaml")
            failures += 1
        elif cname and f"name: {cname}" not in cy.read_text(encoding="utf-8"):
            print(f"[FAIL] {cid}: context.contract={cname} 与 contract.yaml 契约名不一致")
            failures += 1
    print(f"[ OK ] {cid}")

sys.exit(1 if failures else 0)
