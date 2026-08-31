#!/usr/bin/env python3
"""pack_case.py —— 候选 finding → 用例五文件草稿，写入 harvest/inbox/<id>/。

draft 定位：不是半成品用例，是「线索 + 移植 blueprint」。accept = 承诺参照真实案例
移植重写一个可编译用例；rewrite 许可的仓必须重写表达，不得直接复制。

五文件：src/（enclosing file 原样或 before 切片）、CMakeLists.txt、golden.json 草稿、
contract.yaml（空）、notes.md（溯源表 + 缺陷描述 + 真实修复 diff + 移植要点 + accept 清单）。

M1 骨架：pr-mining 候选用 evidence.before_slice 落 src/ 草稿；真实函数边界抽取留给 v0.2。
策略 2：消费候选顶层 dep_count（外部依赖数，缺省 None 不崩）——notes 溯源表记录、
dep_count==0 标题打 🟢 零依赖候选标记、>=10 提示只做 PR/diff 形态评审；打包按 dep_count 升序。
"""
import argparse
import datetime
import json
import os
import re
import sys

# 识别切片内函数定义时要排除的 C/C++ 控制关键字（它们也带 (…) { 形态）
_C_KEYWORDS = {
    "if", "for", "while", "switch", "catch", "return", "sizeof", "alignof",
    "decltype", "static_assert", "do", "else", "case", "goto", "typedef",
}


def _ensure_utf8_streams():
    # CI runner 默认 LANG=C 时，stdout/stderr 是 ascii 编码，写中文会触发
    # UnicodeEncodeError 并被 runner 流处理放大成 RecursionError。强制 utf-8 兜底。
    for s in (sys.stdout, sys.stderr):
        if hasattr(s, "reconfigure"):
            try:
                s.reconfigure(encoding="utf-8", errors="replace")  # type: ignore[attr-defined]
            except Exception:
                pass


def _extern_symbols(before):
    """启发式列出 before 切片依赖的外部符号（与 pr_mine._extern_symbols 同款，自包含各留一份）。

    返回 (外部函数, 大写宏, 外部类型)：被调用但未定义的函数名；出现但未 #define 的
    全大写宏；类型级依赖（struct/union/enum 标签引用但无定义体、首字母大写或 _t
    结尾且未定义的标识符）。
    """
    if not before.strip():
        return [], [], []
    called = set(re.findall(r"\b([a-z_][A-Za-z0-9_]*)\s*\(", before))
    # 函数定义粗判：name(...) { 且括号内无 ;{}（调用语句后跟 ; 或表达式，不匹配）
    defined = set(re.findall(r"\b([a-z_][A-Za-z0-9_]*)\s*\([^;{}]*\)\s*\{", before))
    funcs = sorted(called - defined - _C_KEYWORDS)
    macros_all = set(re.findall(r"\b([A-Z][A-Z0-9_]{2,})\b", before))
    macro_defined = set(re.findall(r"^\s*#\s*define\s+([A-Z][A-Z0-9_]+)", before, re.M))
    macros = sorted(macros_all - macro_defined)
    # 类型级依赖（stub 需求的大头，此前漏统计）
    tags_ref = set(re.findall(r"\b(?:struct|union|enum)\s+([A-Za-z_]\w*)\b", before))
    tags_def = set(re.findall(r"\b(?:struct|union|enum)\s+([A-Za-z_]\w*)\s*\{", before))
    typedef_def = set(re.findall(r"\btypedef\b[^;]*?\b([A-Za-z_]\w*)\s*;", before))
    cap_types = set(re.findall(r"\b([A-Z][A-Za-z0-9_]*[a-z][A-Za-z0-9_]*)\b(?!\s*\()", before))
    t_suffix = set(re.findall(r"\b([a-z_][A-Za-z0-9_]*_t)\b(?!\s*\()", before))
    types = sorted((tags_ref - tags_def) | (cap_types | t_suffix) - typedef_def - set(macros))
    return funcs, macros, types


def pack(finding, inbox_root):
    cid = finding["case_id"]
    d = os.path.join(inbox_root, "draft", cid)
    os.makedirs(os.path.join(d, "src"), exist_ok=True)
    ev = finding.get("evidence", {}) or {}
    before = (ev.get("before_slice") or "")
    base = os.path.basename(finding.get("file", "snippet.c"))
    # 新契约字段（采集端另一路实现）：全部 .get 带默认，旧候选不崩
    license_ = finding.get("license") or "unknown"
    port = finding.get("port") or "rewrite"
    track_hint = finding.get("track_hint") or "defect"
    polarity = finding.get("polarity") or "must_find"
    is_contract = track_hint == "contract" or polarity == "must_not_find"
    # 策略 2：外部依赖数（采集端 pr_mine 统计；旧候选无此字段按 None 处理，不崩）
    dep_count = finding.get("dep_count")
    # 编译地板：gcc/cc -fsyntax-only 错误数（权威可编译性信号；旧候选/无编译器为 None）
    compile_errors = finding.get("compile_errors")
    port_label = "direct（宽松许可，可直接移植）" if port == "direct" \
        else "rewrite（只允许参考，必须重写表达）"
    harvested_at = finding.get("harvested_at") or datetime.date.today().isoformat()
    # anchor 在 before 切片中的相对行号（使 SARIF 标注指向真实 bug 行）
    anchor = finding.get("anchor")
    rel_line = None
    if anchor and before:
        for i, ln in enumerate(before.splitlines(), 1):
            if anchor.strip() in ln or ln.strip() in anchor:
                rel_line = i
                break
    # src 草稿：before 切片（含真实 bug 行），anchor 行用注释标出便于人审。
    # 注意：src/ 是原始切片，不可直接编译；移植时 BUG ANCHOR 标记必须删除。
    src_lines = []
    for i, ln in enumerate(before.splitlines(), 1):
        mark = "  // <<< BUG ANCHOR" if i == rel_line else ""
        src_lines.append(ln + mark)
    with open(os.path.join(d, "src", base), "w") as fh:
        fh.write(f"// AUTO-DRAFT from {ev.get('source_repo')} PR #{ev.get('pr')}\n")
        fh.write("\n".join(src_lines) + "\n")
    # CMakeLists 占位：单文件 -c 编译目标
    with open(os.path.join(d, "CMakeLists.txt"), "w") as fh:
        fh.write(f"# AUTO-DRAFT; 真实构建片段由 v0.2 补全\nadd_library({cid}_draft STATIC src/{base})\n")
    # golden 草稿：
    # - 结构对齐 schema/golden.schema.json：must_find/must_not_find 嵌套在 expected 下
    #   （其余必填字段 id/track/... 待 accept 进 cases/ 时补全，草稿阶段从简）。
    # - license/port/源 PR 不进 golden：schema context 段 additionalProperties:false 冻结，
    #   溯源信息只写 notes.md 溯源表。
    if is_contract:
        # contract 候选（误报矿）：must_not_find 骨架，scenario 有就填，契约依据待移植者补
        mnf = {"note": "误报抑制契约，待移植者补 contract.yaml"}
        if finding.get("scenario"):
            mnf = {"scenario": finding["scenario"], **mnf}
        golden = {"expected": {"must_find": [], "must_not_find": [mnf]}}
    else:
        # defect 候选：anchor + 相对行号（来自修复 diff 反推，有依据）
        # severity 归一到 golden schema 枚举（critical/important/minor），映射不了省略
        sev_map = {"blocker": "critical", "critical": "critical", "error": "critical",
                   "major": "critical", "high": "critical",
                   "warning": "important", "medium": "important", "important": "important",
                   "info": "minor", "minor": "minor", "style": "minor", "note": "minor"}
        sev = sev_map.get(str(finding.get("severity") or "").lower())
        mf = {
            "scenario": finding.get("scenario"),
            "file": f"src/{base}",
            "anchor": anchor,
            "line": rel_line,
            "function": finding.get("function"),
            "rationale": finding.get("message"),
        }
        if sev:
            mf["severity"] = sev
        golden = {
            "expected": {
                "must_find": [mf],
                "must_not_find": [],
            },
        }
    with open(os.path.join(d, "golden.json"), "w") as fh:
        json.dump(golden, fh, indent=2, ensure_ascii=False)
    # contract.yaml 空（/case contract 时填）
    with open(os.path.join(d, "contract.yaml"), "w") as fh:
        fh.write("# 人审判 FP 时填写：该 FP 因何契约成立（exemption_pattern）\n")
    # notes：移植 blueprint（溯源表 + 缺陷描述 + 真实修复 diff + 移植要点 + accept 清单）
    raw_patch = (finding.get("raw", {}) or {}).get("patch", "")
    desc = finding.get("rationale") or finding.get("message") or "（候选未带描述，待移植者补）"
    funcs, macros, types = _extern_symbols(before)
    with open(os.path.join(d, "notes.md"), "w") as fh:
        # 🟢 零依赖候选：优先按编译地板（compile_errors==0）判定，无编译器数据时退回 dep_count==0
        zero_ok = (compile_errors == 0) if compile_errors is not None else (dep_count == 0)
        zero_mark = " 🟢 零依赖候选" if zero_ok else ""
        fh.write(f"# {cid}{zero_mark}\n\n")
        fh.write("> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例"
                 "移植重写一个可编译用例。\n\n")
        fh.write("## 溯源\n\n")
        fh.write("| 项 | 值 |\n|---|---|\n")
        fh.write(f"| 源仓 | {ev.get('source_repo')} |\n")
        fh.write(f"| 源 PR | [#{ev.get('pr')}]({ev.get('pr_url')}) |\n")
        fh.write(f"| 许可证 | {license_} |\n")
        fh.write(f"| 移植策略 | {port_label} |\n")
        fh.write(f"| 采集时间 | {harvested_at} |\n")
        fh.write(f"| track 方向 | {track_hint} 候选（polarity={polarity}） |\n")
        # 兼容：make_draft_sarif.sh 用正则从这行提取 dep_count 写进四态表，格式勿改
        dep_label = str(dep_count) if dep_count is not None else "未知（旧候选未带）"
        fh.write(f"| 外部依赖数（dep_count） | {dep_label} |\n")
        ce_label = (f"{compile_errors}（0=切片已达编译地板）" if compile_errors is not None
                    else "未测（采集端无编译器或旧候选）")
        fh.write(f"| 编译错误数（gcc syntax-only） | {ce_label} |\n\n")
        fh.write(f"- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）\n")
        fh.write(f"- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）\n")
        # 兼容：make_draft_sarif.sh 用正则从这行提取 PR 号/链接写进 SARIF 与四态表，格式勿改
        fh.write(f"- 源 PR: #{ev.get('pr')} ({ev.get('pr_url')})\n")
        fh.write(f"- 候选初判 scenario: **{finding.get('scenario')}（候选猜测，待 LLM/人审定，非真值）**\n")
        fh.write(f"- 候选初判锚点行: {rel_line}（原始 PR diff 行 {ev.get('anchor_line')}；PR 修复前的代码，待确认是否为 bug）\n\n")
        fh.write("## 缺陷描述与触发条件\n\n")
        fh.write(f"{desc}\n\n")
        fh.write("- 触发条件（一句话复述，移植者补写）：\n\n")
        fh.write("> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写\n\n")
        fh.write("## 真实修复 diff（PR 改了什么）\n\n")
        if raw_patch:
            fh.write("```diff\n" + raw_patch[:4000] + "\n```\n\n")
        else:
            fh.write("（无 diff）\n\n")
        fh.write("## 移植要点\n\n")
        if funcs or macros or types:
            fh.write("before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：\n\n")
            for s in funcs:
                fh.write(f"- 外部函数：`{s}`\n")
            for s in macros:
                fh.write(f"- 大写宏：`{s}`\n")
            for s in types:
                fh.write(f"- 外部类型：`{s}`\n")
        else:
            fh.write("外部符号依赖（启发式未列出，移植者补）：\n\n")
            fh.write("- 外部函数：（待填）\n")
            fh.write("- 外部类型/宏：（待填）\n")
        fh.write("\n- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。\n")
        fh.write("- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。\n")
        if isinstance(dep_count, int) and dep_count >= 10:
            fh.write("- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。\n")
        fh.write("\n")
        if is_contract:
            fh.write("## 为什么契约安全\n\n")
            fh.write("（模板，移植者填：此处报出为什么是误报？豁免契约是什么？"
                     "与 contract.yaml 的 contract.name / exemption_pattern 对应。）\n\n")
        fh.write("## accept 检查清单\n\n")
        fh.write("- [ ] 编译通过（重写后的 src/ 可独立编译）\n")
        fh.write("- [ ] golden anchor 真实存在于 src/\n")
        fh.write("- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）\n")
        fh.write("- [ ] license 策略已遵守（rewrite 仓代码已重写表达）\n")
        fh.write("- [ ] `// <<< BUG ANCHOR` 标记已清除\n")
        fh.write("- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））\n\n")
        fh.write("## 接受后流程（accept → case）\n\n")
        fh.write(f"1. 完成上面检查清单后评论 `/case accept {cid}` → 本草稿移入 `cases/{track_hint}/{cid}/`（五文件齐备）\n")
        fh.write("2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）\n")
        fh.write("3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告\n")
        fh.write("4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden\n")


def main():
    _ensure_utf8_streams()
    ap = argparse.ArgumentParser()
    ap.add_argument("--candidates", required=True)
    ap.add_argument("--inbox", required=True)
    args = ap.parse_args()
    with open(args.candidates) as fh:
        cands = json.load(fh)
    # 策略 2：按可编译性升序打包（编译地板优先，其次 dep_count 启发式，都缺=旧候选排最后），
    # 使 inbox/draft 目录列举顺序即移植优先级。稳定排序，同键保持原顺序。
    def _prio(f):
        if f.get("compile_errors") is not None:
            return (0, f["compile_errors"], f.get("dep_count") or 0)
        if f.get("dep_count") is not None:
            return (1, f["dep_count"], 0)
        return (2, 0, 0)
    cands.sort(key=_prio)
    for f in cands:
        pack(f, args.inbox)
    sys.stderr.write(f"[pack_case] {len(cands)} drafts -> {args.inbox}/draft/\n")


if __name__ == "__main__":
    main()
