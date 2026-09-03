#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""cpp-review-bench 评分器（两层匹配 + 四态 + 汇总）。

设计口径见 docs/design-v0.4.md §4（两层匹配协议）。

L1 确定性匹配（per finding）:
  - must_find 命中：scenario 家族匹配（finding.scenario 为 null 时不强制）AND
    file 精确 AND (anchor 去空白子串匹配 OR line ∈ [gline±tol]) AND
    function 精确（golden 含 function 时）
    - finding 无 anchor 时不得走 anchor 分支，只能走 line±tolerance（I1 修复）
    - twin 点位（must_find 与 must_not_find 同 file+anchor，仅靠 function 区分）上
      finding 无 function 时不计命中，只计 FP（I2 修复，保守口径）
  - must_not_find 违反（FP）：file 精确 AND anchor 去空白子串匹配（同一语句被报出）
    - contract 轨 + 注入了 contract.yaml 仍报 => 契约违反（权重 > 裸 FP）
    - defect 轨 => 裸 FP

`run` 入口对每个 findings 文件做 schema/findings.schema.json 校验（--no-validate 可关），
缺 track/case_id 的非 findings 文件（如 _summary.json）跳过并告警。

L2 语义判等（可选，默认关）：rationale vs finding.message 的轻量 judge，此处预留接口。

四态（per case）:
  PASS / FN（漏报）/ FP（误报：裸 FP 或契约违反）/ EXTRA（未吸收的多余 finding）

汇总:
  per-track pass 率；裸 FP vs 契约违反分列；severity 分级正确率；verified 计数。

自检（design §7.1）:
  python3 tools/eval.py selftest
  —— 对构造 findings（含 1 FP + 1 FN + 1 契约违反）验证判定正确。
"""
import argparse
import json
import re
import sys
from pathlib import Path

import jsonschema

ROOT = Path(__file__).resolve().parent.parent

FINDINGS_SCHEMA = None


def findings_schema() -> dict:
    """延迟加载 schema/findings.schema.json（run 入口校验用）。"""
    global FINDINGS_SCHEMA
    if FINDINGS_SCHEMA is None:
        FINDINGS_SCHEMA = load_json(ROOT / "schema/findings.schema.json")
    return FINDINGS_SCHEMA


def validate_findings_doc(doc) -> list:
    """用 schema/findings.schema.json 校验一份 findings 文档，返回错误列表（空 = 合规）。"""
    v = jsonschema.Draft202012Validator(findings_schema())
    return sorted(v.iter_errors(doc), key=lambda e: list(e.absolute_path))


def is_findings_doc(doc) -> bool:
    """是否 findings 文档（缺 track/case_id 的 json 如 _summary.json 汇总产物不算）。"""
    return isinstance(doc, dict) and "track" in doc and "case_id" in doc


def norm(s: str) -> str:
    return re.sub(r"\s+", "", s or "")


def scenario_family_match(golden_s: str, finding_s) -> bool:
    """scenario 家族匹配：golden 用 cwe-XXX；finding 为 null 时视为不强制匹配。

    支持组合场景（schema 允许 `cwe-A+cwe-B`）：golden 与 finding 任一成员分量相交即命中，
    避免 `cwe-190+cwe-787` 与工具报的单个 `cwe-190`/`cwe-787` 永远无法匹配（恒 FN）。
    """
    if finding_s is None:
        return True
    if golden_s == finding_s:
        return True
    g_parts = set(str(golden_s).split("+"))
    f_parts = set(str(finding_s).split("+"))
    return bool(g_parts & f_parts)


def function_conflict(g: dict, f: dict) -> bool:
    """golden 与 finding 都给出 function 且不同名 → 明确不是同一点位。
    任一方未给出时按「不能排除」处理（保守，仍计匹配）。"""
    gfunc = g.get("function")
    ffunc = f.get("function")
    return bool(gfunc and ffunc and gfunc != ffunc)


def load_json(p: Path):
    return json.loads(p.read_text(encoding="utf-8"))


def locate_gline(case_dir: Path | None, g: dict) -> int | None:
    """把 golden 的 anchor 定位到 case 源文件行号：anchor 去空白后是某行去空白内容的
    子串即取该行（首个匹配行）。文件/anchor 找不到时返回 None（line 容差兜底不生效）。"""
    if case_dir is None:
        return None
    gfile = g.get("file")
    ganchor = norm(g.get("anchor", ""))
    if not gfile or not ganchor:
        return None
    src = case_dir / gfile
    if not src.is_file():
        return None
    for i, line in enumerate(
            src.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        if ganchor in norm(line):
            return i
    return None


def finding_hits_must(g: dict, f: dict, gline: int | None) -> bool:
    """must_find 命中判定：scenario 家族 + function 精确（golden 有 function 时）+
    （anchor 去空白互为子串，优先；或 finding.line ∈ [gline±line_tolerance]，兜底）。"""
    if not scenario_family_match(g["scenario"], f.get("scenario")):
        return False
    gfunc = g.get("function")
    if gfunc and f.get("function") and f.get("function") != gfunc:
        return False
    ganchor = norm(g.get("anchor", ""))
    fanchor = norm(f.get("anchor", ""))
    # I1 修复：finding 无 anchor 时不得走 anchor 分支（"" 恒为子串会假命中），
    # 只能走下面的 line±tolerance 兜底；anchor 与 line 都没有则不得判命中
    if ganchor and fanchor and (ganchor in fanchor or fanchor in ganchor):
        return True
    # line±tolerance 兜底（仅当调用方解析出 gline 且 finding 带 line）
    if gline is not None and f.get("line"):
        tol = g.get("line_tolerance", 3)
        try:
            return abs(int(f["line"]) - gline) <= tol
        except (TypeError, ValueError):
            return False
    return False


def anchors_overlap(a, b) -> bool:
    """anchor 去空白后互为子串即视为重叠（与 L1 匹配同口径）。"""
    a, b = norm(a or ""), norm(b or "")
    return bool(a and b and (a in b or b in a))


def eval_case(golden: dict, findings_doc: dict | None, contract_injected: bool,
              case_dir: Path | None = None) -> dict:
    track = golden["track"]
    cid = golden["id"]
    g_must = golden["expected"]["must_find"]
    g_not = golden["expected"]["must_not_find"]
    findings = (findings_doc or {}).get("findings", [])

    # 索引：file -> [findings]
    by_file = {}
    for f in findings:
        by_file.setdefault(f.get("file"), []).append(f)

    # 预定位每条 must_find 的 golden 行号（line 容差兜底用）
    glines = [locate_gline(case_dir, g) for g in g_must]

    # twin 点位标记：同 file 且 anchor 重叠的 must_not_find 与 must_find 成对存在
    # （如 r04/r09/r14 的受界/未界 twin），区分只靠 function
    twin = [any(g2.get("file") == g["file"]
                and anchors_overlap(g2.get("anchor", ""), g.get("anchor", ""))
                for g2 in g_not)
            for g in g_must]

    def hits_must(gi: int, g: dict, f: dict) -> bool:
        # I2 修复：twin 点位上 finding 无 function 时无法自证命中的是缺陷点还是
        # 受保护点，保守处理——不计 must_find 命中（仍会按 must_not_find 计 FP）；
        # finding 带 function 时按 function 精确区分，行为不变
        if twin[gi] and not f.get("function"):
            return False
        return finding_hits_must(g, f, glines[gi])

    # must_find 命中判定
    must_hit = []
    for gi, g in enumerate(g_must):
        gf = g["file"]
        hit = False
        for f in by_file.get(gf, []):
            if hits_must(gi, g, f):
                hit = True
                break
        must_hit.append({"scenario": g["scenario"], "hit": hit,
                         "severity": g.get("severity")})

    # must_not_find 违反判定（按 file+anchor）
    not_viol = []
    for g in g_not:
        gf = g.get("file")
        ganchor = norm(g.get("anchor", ""))
        violated = False
        for f in by_file.get(gf, []):
            if function_conflict(g, f):
                continue   # 锚点形同但函数不同名，非同一违反点（如 r04 受界/未界 twin）
            if not ganchor:
                # 无 anchor 的 must_not_find：file 内任意 finding 即视为违反
                violated = True
                break
            fanchor = norm(f.get("anchor", ""))
            if fanchor and (ganchor in fanchor or fanchor in ganchor):
                violated = True
                break
        kind = None
        if violated:
            if track == "contract" and contract_injected:
                kind = "contract_violation"   # 权重 > 裸 FP
            else:
                kind = "bare_fp"
        not_viol.append({"scenario": g.get("scenario"), "violated": violated,
                         "kind": kind})

    # EXTRA：未被任何 must_find/must_not_find 吸收的 finding
    # 注意：absorbed 必须用 (file, local_index) 元组键，避免不同文件局部索引冲突
    absorbed = set()
    for gi, g in enumerate(g_must):
        gf = g["file"]
        for fi, f in enumerate(by_file.get(gf, [])):
            if (gf, fi) in absorbed:
                continue
            if hits_must(gi, g, f):
                absorbed.add((gf, fi))
    for g in g_not:
        gf = g.get("file"); ganchor = norm(g.get("anchor", ""))
        for fi, f in enumerate(by_file.get(gf, [])):
            if (gf, fi) in absorbed:
                continue
            if function_conflict(g, f):
                continue
            fanchor = norm(f.get("anchor", ""))
            if (not ganchor) or (fanchor and (ganchor in fanchor or fanchor in ganchor)):
                absorbed.add((gf, fi))
    extra = []
    for gf, flist in by_file.items():
        for fi, f in enumerate(flist):
            if (gf, fi) not in absorbed:
                extra.append({"file": gf, "anchor": f.get("anchor"),
                              "scenario": f.get("scenario")})

    # 四态判定
    fn = any(not h["hit"] for h in must_hit)
    fp = any(v["violated"] for v in not_viol)
    has_extra = len(extra) > 0
    if fn and fp:
        state = "FN+FP"
    elif fn:
        state = "FN"
    elif fp:
        state = "FP"
    elif has_extra:
        state = "EXTRA"
    else:
        state = "PASS"

    # severity 正确率：命中的 must_find 中，severity 一致比例
    sev_total = sev_ok = 0
    for gi, g in enumerate(g_must):
        if must_hit[gi]["hit"]:
            sev_total += 1
            # 找到命中该 g 的 finding 的 severity
            gf = g["file"]
            for f in by_file.get(gf, []):
                if hits_must(gi, g, f):
                    if f.get("severity") == g.get("severity"):
                        sev_ok += 1
                    break

    verified = sum(1 for f in findings if f.get("verified"))

    return {
        "case_id": cid,
        "track": track,
        "state": state,
        "must_find_total": len(g_must),
        "must_find_hit": sum(1 for h in must_hit if h["hit"]),
        "must_not_total": len(g_not),
        "must_not_violated": sum(1 for v in not_viol if v["violated"]),
        "contract_violations": sum(1 for v in not_viol if v["kind"] == "contract_violation"),
        "bare_fp": sum(1 for v in not_viol if v["kind"] == "bare_fp"),
        "extra": len(extra),
        "severity_total": sev_total,
        "severity_ok": sev_ok,
        "verified": verified,
        "n_findings": len(findings),
        "detail": {"must_hit": must_hit, "not_viol": not_viol, "extra": extra},
    }


def eval_all(track_filter=None, contract_injected=True) -> list:
    results = []
    for gj in sorted(ROOT.glob("cases/*/*/golden.json")):
        case_dir = gj.parent
        golden = load_json(gj)
        track = golden["track"]
        if track_filter and track != track_filter:
            continue
        cid = case_dir.name
        # 找 findings：cases/<track>/<cid>/findings.json（消费方产物）或留空（=无工具结果）
        fdoc = None
        fpath = case_dir / "findings.json"
        if fpath.is_file():
            fdoc = load_json(fpath)
        # contract 轨是否注入契约：由评测方通过 --no-contract 显式控制（默认已注入）。
        # 区分「裸跑 FP」与「注入契约后仍报（契约违反）」，见 design §4 附加维度。
        results.append(eval_case(golden, fdoc, contract_injected, case_dir))
    return results


def summarize(results: list) -> dict:
    by_track = {}
    for r in results:
        by_track.setdefault(r["track"], []).append(r)
    out = {}
    for track, rs in by_track.items():
        n = len(rs)
        passed = sum(1 for r in rs if r["state"] == "PASS")
        fn = sum(1 for r in rs if "FN" in r["state"])
        fp = sum(1 for r in rs if "FP" in r["state"])
        extra = sum(1 for r in rs if r["state"] == "EXTRA")
        bare = sum(r["bare_fp"] for r in rs)
        cv = sum(r["contract_violations"] for r in rs)
        sev_t = sum(r["severity_total"] for r in rs)
        sev_o = sum(r["severity_ok"] for r in rs)
        ver = sum(r["verified"] for r in rs)
        mft = sum(r["must_find_total"] for r in rs)
        mfh = sum(r["must_find_hit"] for r in rs)
        out[track] = {
            "cases": n,
            "pass": passed,
            "pass_rate": round(passed / n, 4) if n else 0,
            "fn_cases": fn,
            "fp_cases": fp,
            "extra_cases": extra,
            "bare_fp": bare,
            "contract_violations": cv,
            "recall": round(mfh / mft, 4) if mft else 0,
            "severity_accuracy": round(sev_o / sev_t, 4) if sev_t else 0,
            "verified": ver,
        }
    return out


def cmd_eval(args):
    results = eval_all(args.track, contract_injected=args.contract)
    summ = summarize(results)
    print(json.dumps({"summary": summ, "cases": results},
                     ensure_ascii=False, indent=2))


def cmd_run(args):
    """跑指定工具的一组 findings（目录或单个文件），输出汇总。"""
    fpath = Path(args.findings)
    paths = []
    if fpath.is_dir():
        for p in sorted(fpath.glob("*.json")):
            if p.name.endswith(".raw.json"):
                continue  # llm_review 的模型原始输出旁车（含 flow/reason），不是归一化 findings
            paths.append(p)
    else:
        paths.append(fpath)
    docs = []
    for p in paths:
        doc = load_json(p)
        if not is_findings_doc(doc):
            # S1 修复：缺 track/case_id 的 json（如 harvest 的 _summary.json）跳过并告警，
            # 不再 KeyError 崩溃
            print(f"[WARN] 跳过非 findings 文件（缺 track/case_id 字段）: {p}",
                  file=sys.stderr)
            continue
        docs.append((p, doc))
    if not args.no_validate:
        bad = 0
        for p, doc in docs:
            errs = validate_findings_doc(doc)
            if errs:
                bad += 1
                print(f"[FAIL] findings 不合 schema/findings.schema.json: {p}",
                      file=sys.stderr)
                for e in errs[:10]:
                    loc = "/".join(str(x) for x in e.absolute_path) or "<root>"
                    print(f"  - {loc}: {e.message}", file=sys.stderr)
        if bad:
            print(f"[FAIL] {bad} 个 findings 文件不合规，评测中止"
                  f"（--no-validate 可跳过校验）", file=sys.stderr)
            sys.exit(1)
    results = []
    for p, doc in docs:
        gj = ROOT / "cases" / doc["track"] / doc["case_id"] / "golden.json"
        if not gj.is_file():
            print(f"[WARN] 无 golden: {doc['track']}/{doc['case_id']}", file=sys.stderr)
            continue
        golden = load_json(gj)
        contract_injected = args.contract
        results.append(eval_case(golden, doc, contract_injected, gj.parent))
    print(json.dumps({"summary": summarize(results), "cases": results},
                     ensure_ascii=False, indent=2))


def synthetic_findings() -> dict:
    """构造 findings：对 c08 注入 1 契约违反 + 1 FP；对 c01 注入 1 FN（漏报 must_find）。"""
    # c08 契约违反：在 u2u_fields.c 的访问器裸读处报 cwe-125（must_not_find，contract 注入）
    # c08 裸 FP：在 u2u_validate.c 的 u2u_payload_offset_checked 处报（非 golden 任何点）= EXTRA
    # c01 FN：完全不报 must_find（guti_group_size 回绕）=> FN
    return {
        "c08": {
            "tool": "synthetic", "track": "contract", "case_id": "c08-protocol-offset-parse",
            "version": "selftest",
            "findings": [
                {"file": "src/u2u_fields.c",
                 "anchor": "return frame[11 + src_len_at(frame)];",
                 "scenario": "cwe-125", "severity": "important",
                 "function": "dst_len_at", "line": 11},
                {"file": "src/u2u_validate.c",
                 "anchor": "return u2u_payload_offset(frame);",
                 "scenario": "cwe-125", "severity": "minor",
                 "function": "u2u_payload_offset_checked", "line": 20},
            ],
        },
        "c01": {
            "tool": "synthetic", "track": "contract", "case_id": "c01-upstream-nullguard",
            "version": "selftest",
            "findings": [],  # 故意漏报 must_find => FN
        },
        # line 容差兜底：anchor 故意与 golden 互不子串，但 line=42 落在
        # golden anchor 实际行（guti.c:40）±line_tolerance(3) 内 => 兜底命中 => PASS
        "c01-line": {
            "tool": "synthetic", "track": "contract", "case_id": "c01-upstream-nullguard",
            "version": "selftest",
            "findings": [
                {"file": "src/guti.c",
                 "anchor": "/* line-fallback probe */",
                 "scenario": "cwe-190", "severity": "important",
                 "function": "guti_group_size", "line": 42},
            ],
        },
    }


def cmd_selftest(args):
    syn = synthetic_findings()
    ok = True
    # c08：应判 FP（含 1 契约违反）+ 1 EXTRA
    g8 = load_json(ROOT / "cases/contract/c08-protocol-offset-parse/golden.json")
    r8 = eval_case(g8, syn["c08"], contract_injected=True)
    if not (r8["state"] in ("FP", "FN+FP") and r8["contract_violations"] == 1
            and r8["extra"] == 1):
        print(f"[FAIL] c08 期望 FP+契约违反1+EXTRA1, 实际 {r8['state']} "
              f"cv={r8['contract_violations']} extra={r8['extra']}")
        ok = False
    else:
        print(f"[ OK ] c08: {r8['state']} 契约违反={r8['contract_violations']} "
              f"EXTRA={r8['extra']}")
    # c01：应判 FN（must_find 全漏）
    g1 = load_json(ROOT / "cases/contract/c01-upstream-nullguard/golden.json")
    r1 = eval_case(g1, syn["c01"], contract_injected=True)
    if not (r1["state"] in ("FN", "FN+FP") and r1["must_find_hit"] == 0):
        print(f"[FAIL] c01 期望 FN, 实际 {r1['state']} hit={r1['must_find_hit']}")
        ok = False
    else:
        print(f"[ OK ] c01: {r1['state']} must_find_hit={r1['must_find_hit']}")
    # 额外验证：契约未注入时 c08 同一 finding 应降为裸 FP（权重差异）
    r8b = eval_case(g8, syn["c08"], contract_injected=False)
    if r8b["contract_violations"] != 0 or r8b["bare_fp"] != 1:
        print(f"[FAIL] 未注入契约时 c08 应记裸 FP, 实际 cv={r8b['contract_violations']} bare={r8b['bare_fp']}")
        ok = False
    else:
        print(f"[ OK ] c08(未注入契约): 裸 FP={r8b['bare_fp']}（权重 < 契约违反）")
    # line 容差兜底：anchor 不命中但 line 在 gline±tolerance 内 => PASS（且被 must_find 吸收，无 EXTRA）
    r1l = eval_case(g1, syn["c01-line"], contract_injected=True,
                    case_dir=ROOT / "cases/contract/c01-upstream-nullguard")
    if not (r1l["state"] == "PASS" and r1l["must_find_hit"] == 1 and r1l["extra"] == 0):
        print(f"[FAIL] c01-line 期望 PASS（line 容差兜底命中）, 实际 {r1l['state']} "
              f"hit={r1l['must_find_hit']} extra={r1l['extra']}")
        ok = False
    else:
        print(f"[ OK ] c01-line: {r1l['state']}（anchor 未命中，line±tolerance 兜底命中）")

    # I1 回归：无 anchor 且无 line 的 finding 不得凭空命中 must_find（"" 恒为子串的缺陷）
    c01_dir = ROOT / "cases/contract/c01-upstream-nullguard"
    f_noanchor = {"tool": "synthetic", "track": "contract",
                  "case_id": "c01-upstream-nullguard",
                  "findings": [{"file": "src/guti.c", "scenario": "cwe-190",
                                "severity": "important", "function": "guti_group_size"}]}
    r1i = eval_case(g1, f_noanchor, contract_injected=True, case_dir=c01_dir)
    if r1i["must_find_hit"] != 0 or "FN" not in r1i["state"]:
        print(f"[FAIL] I1 无 anchor/line 的 finding 不得命中, 实际 {r1i['state']} "
              f"hit={r1i['must_find_hit']}")
        ok = False
    else:
        print(f"[ OK ] I1: 无 anchor/line finding 未命中 must_find（{r1i['state']}）")
    # I1 对照：无 anchor 但 line 在容差内仍允许经 line±tolerance 命中
    f_noanchor_line = {"tool": "synthetic", "track": "contract",
                       "case_id": "c01-upstream-nullguard",
                       "findings": [{"file": "src/guti.c", "scenario": "cwe-190",
                                     "severity": "important",
                                     "function": "guti_group_size", "line": 42}]}
    r1j = eval_case(g1, f_noanchor_line, contract_injected=True, case_dir=c01_dir)
    if not (r1j["state"] == "PASS" and r1j["must_find_hit"] == 1):
        print(f"[FAIL] I1 对照：无 anchor 但 line 容差内应命中, 实际 {r1j['state']} "
              f"hit={r1j['must_find_hit']}")
        ok = False
    else:
        print(f"[ OK ] I1 对照: 无 anchor + line 容差内 => {r1j['state']}（line 兜底保留）")

    # I2 回归：r04 twin（must_find 与 must_not_find 同 file+anchor，仅靠 function 区分）
    g4 = load_json(ROOT / "cases/defect/r04-oob-write-stack/golden.json")
    f_twin_nofunc = {"tool": "synthetic", "track": "defect",
                     "case_id": "r04-oob-write-stack",
                     "findings": [{"file": "src/recv.c",
                                   "anchor": "memcpy(buf, payload, len);",
                                   "scenario": "cwe-787", "severity": "important"}]}
    r4a = eval_case(g4, f_twin_nofunc, contract_injected=True)
    # 状态为 FN+FP：finding 只报了受保护点（FP），缺陷点确实漏报（FN）——关键是 TP 不被虚抬
    if not (r4a["must_find_hit"] == 0 and r4a["bare_fp"] == 1
            and r4a["state"] == "FN+FP" and r4a["extra"] == 0):
        print(f"[FAIL] I2 无 function 的 twin finding 应只计 FP 不计 TP, 实际 "
              f"{r4a['state']} hit={r4a['must_find_hit']} bare={r4a['bare_fp']} "
              f"extra={r4a['extra']}")
        ok = False
    else:
        print(f"[ OK ] I2: 无 function twin finding => {r4a['state']} "
              f"hit={r4a['must_find_hit']} bare_fp={r4a['bare_fp']}（不计 TP）")
    # I2 对照①：finding 带 function=r04_recv（缺陷点）应正常命中且不算 FP（行为不回归）
    f_twin_hit = {"tool": "synthetic", "track": "defect",
                  "case_id": "r04-oob-write-stack",
                  "findings": [{"file": "src/recv.c",
                                "anchor": "memcpy(buf, payload, len);",
                                "scenario": "cwe-787", "severity": "important",
                                "function": "r04_recv"}]}
    r4b = eval_case(g4, f_twin_hit, contract_injected=True)
    if not (r4b["state"] == "PASS" and r4b["must_find_hit"] == 1
            and r4b["bare_fp"] == 0):
        print(f"[FAIL] I2 对照：function=r04_recv 应 PASS, 实际 {r4b['state']} "
              f"hit={r4b['must_find_hit']} bare={r4b['bare_fp']}")
        ok = False
    else:
        print(f"[ OK ] I2 对照①: function=r04_recv => {r4b['state']}（function 精确区分）")
    # I2 对照②：finding 带 function=r04_recv_ok（受保护点）应只计 FP
    f_twin_fp = {"tool": "synthetic", "track": "defect",
                 "case_id": "r04-oob-write-stack",
                 "findings": [{"file": "src/recv.c",
                               "anchor": "memcpy(buf, payload, len);",
                               "scenario": "cwe-787", "severity": "important",
                               "function": "r04_recv_ok"}]}
    r4c = eval_case(g4, f_twin_fp, contract_injected=True)
    if not (r4c["state"] == "FN+FP" and r4c["must_find_hit"] == 0
            and r4c["bare_fp"] == 1):
        print(f"[FAIL] I2 对照：function=r04_recv_ok 应 FP, 实际 {r4c['state']} "
              f"hit={r4c['must_find_hit']} bare={r4c['bare_fp']}")
        ok = False
    else:
        print(f"[ OK ] I2 对照②: function=r04_recv_ok => {r4c['state']}（function 冲突排除命中）")

    # findings schema 校验：合规/不合规各一例 + 非 findings 文档识别
    doc_ok = {"tool": "synthetic", "track": "defect", "case_id": "r04-oob-write-stack",
              "findings": [{"file": "src/recv.c", "anchor": "memcpy(buf, payload, len);",
                            "scenario": "cwe-787", "severity": "important",
                            "function": "r04_recv", "line": 10,
                            "message": "m", "verified": True}]}
    doc_bad = json.loads(json.dumps(doc_ok))
    doc_bad["findings"][0]["severity"] = "style"   # 非枚举值
    doc_bad["findings"][0]["column"] = 3           # 多余字段
    doc_summary = {"summary": {}, "cases": []}     # 汇总产物，非 findings
    errs_ok = validate_findings_doc(doc_ok)
    errs_bad = validate_findings_doc(doc_bad)
    if errs_ok or not errs_bad or is_findings_doc(doc_summary):
        print(f"[FAIL] schema 校验断言: 合规例错误={len(errs_ok)}（应 0）, "
              f"不合规例错误={len(errs_bad)}（应 >0）, "
              f"summary 被误判为 findings={is_findings_doc(doc_summary)}（应 False）")
        ok = False
    else:
        print(f"[ OK ] schema 校验: 合规例通过, 不合规例报 {len(errs_bad)} 错, "
              f"summary 产物正确识别为非 findings")

    print("SELFTEST", "PASS" if ok else "FAIL")
    sys.exit(0 if ok else 1)


def main():
    ap = argparse.ArgumentParser(description="cpp-review-bench 评分器")
    sub = ap.add_subparsers(dest="cmd", required=True)
    e = sub.add_parser("eval", help="评测仓内全部 cases（读取各 case findings.json）")
    e.add_argument("--track", choices=["contract", "defect"], default=None)
    e.add_argument("--no-contract", dest="contract", action="store_false", default=True,
                   help="contract 轨按「未注入契约」口径评测（must_not_find 命中记为裸 FP 而非契约违反）")
    e.set_defaults(func=cmd_eval)
    r = sub.add_parser("run", help="评测外部 findings（文件或目录）")
    r.add_argument("findings")
    r.add_argument("--no-contract", dest="contract", action="store_false", default=True,
                   help="contract 轨按「未注入契约」口径评测（must_not_find 命中记为裸 FP 而非契约违反）")
    r.add_argument("--no-validate", dest="no_validate", action="store_true", default=False,
                   help="跳过 findings 对 schema/findings.schema.json 的校验（默认校验，不合规即失败）")
    r.set_defaults(func=cmd_run)
    s = sub.add_parser("selftest", help="构造 findings 自检（1FP+1FN+1契约违反）")
    s.set_defaults(func=cmd_selftest)
    args = ap.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
