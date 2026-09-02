#!/usr/bin/env python3
"""LLM/Agent 评审消费入口：把仓内 case 喂给 LLM 评审，产出归一化 findings。

用法：
    AI_REVIEW_API_KEY=sk-xxx python3 tools/llm_review.py \
        --cases c01-upstream-nullguard,r04-oob-write-stack --out /tmp/llm_findings
    python3 tools/eval.py run /tmp/llm_findings

口径：
  - 每个 case 一次独立评审：src/ 全部源码内联进 prompt；contract 轨附加 contract.yaml
    原文作为「已确认契约」，测 FP 抑制；defect 轨不注入。
  - 模型按 schema/findings.schema.json 的字段输出 JSON（file+anchor 必填，
    scenario 可为 null，severity 为 critical/important/minor 三档）。
  - anchor 缺失时用 file+line 从源文件取该行文本补齐。
  - 仅依赖 Python 标准库；接口为 OpenAI 兼容的 Chat Completions。

环境变量：
  AI_REVIEW_API_KEY   必填
  AI_REVIEW_BASE_URL  默认 https://api.deepseek.com（任意 OpenAI 兼容端点）
  AI_REVIEW_MODEL     默认 deepseek-chat
"""
import argparse
import json
import os
import re
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SCENARIO_RE = re.compile(r"^(cwe-[0-9]+|build|logic)(\+cwe-[0-9]+)*$")
SEVERITIES = {"critical", "important", "minor"}

PROMPT_TMPL = """你是一名严格的 C/C++ 代码评审器。评审下面这个小模块的完整源代码，找出真实的、有代码证据的缺陷。

评审范围（重点关注但不限于）：缓冲区越界读写、空指针解引用、整数溢出/回绕、
double free / use-after-free / 内存泄漏、锁遗漏、有符号/无符号比较错误、
off-by-one、长度/索引来自不可信输入导致的越界。

纪律（违反将扣分）：
1. 只报有具体代码行证据的缺陷；不报风格问题、不报无证据的猜测。
2. anchor 必须是缺陷真正发生的那行语句原文（越界/解引用/回绕/释放发生的赋值、
   拷贝、解引用、比较语句），逐字照抄、去掉行首空白。不要把 anchor 挂在变量
   声明行、循环头或函数签名上——评审锚点按「缺陷语句」判定。
3. 同一形态在受保护与未受保护两处都出现时，只报未受保护处（按所在函数区分）。
3. scenario 用 CWE 编号标注（如 cwe-787、cwe-476、cwe-190、format）；不确定就填 null。
4. severity 三档的选取纪律：默认 **important**（真实缺陷，触发有条件或影响受限）；
   仅当无前置条件即可直接利用/必然数据损坏时才 **critical**；仅影响面微小的边角
   问题才 **minor**。不要因缺陷「听起来严重」就升格 critical。{contract_section}

输出格式：只输出一个 JSON 对象，不要任何 markdown 围栏或其他文字：
{{"findings":[{{"file":"src/xxx.c","line":12,"anchor":"代码行原文","function":"函数名","scenario":"cwe-xxx 或 null","severity":"critical|important|minor","reason":"一句话理由","flow":[{{"file":"src/xxx.c","line":3,"message":"来源点：数据从哪来/在哪分配/在哪赋值"}},{{"file":"src/xxx.c","line":7,"message":"中间传播步骤"}},{{"file":"src/xxx.c","line":12,"message":"缺陷触发点（与本 finding 的 anchor 同处）"}}]}}]}}
flow 为有序证据链（2-6 步）：从来源（入参/分配/读取）到缺陷触发点，逐步给出 file+line+一句话；
跨函数/跨文件时务必逐跳列出。无把握给全链时可省略 flow 键。
若无缺陷，输出 {{"findings":[]}}。

========== 被评审源码 ==========
{sources}
========== 源码结束 =========="""

CONTRACT_SECTION = """
5. 以下契约/安全保证已经人工复核确认成立（来自 contract.yaml）。对契约覆盖的
   代码点不得发出告警——它们看似可疑但在契约下是安全的：
```yaml
{contract_yaml}
```"""


def collect_sources(case_dir: Path) -> str:
    """把 case src/ 下全部源文件拼成「路径 + 内容」文本块。"""
    parts = []
    for p in sorted((case_dir / "src").rglob("*")):
        if p.is_file() and p.suffix in (".c", ".h", ".cpp", ".cc", ".hpp"):
            rel = p.relative_to(case_dir)
            parts.append(f"----- {rel} -----\n{p.read_text(encoding='utf-8')}")
    return "\n".join(parts)


def call_llm(base_url: str, api_key: str, model: str, prompt: str,
             temperature: float = 0.0, retries: int = 2) -> str:
    """调用 OpenAI 兼容 Chat Completions，返回 content 文本。"""
    url = base_url.rstrip("/") + "/chat/completions"
    body = {
        "model": model,
        "messages": [
            {"role": "system", "content": "你是严谨的 C/C++ 静态评审专家，输出严格 JSON。"},
            {"role": "user", "content": prompt},
        ],
        "temperature": temperature,
        "max_tokens": 4096,
        "response_format": {"type": "json_object"},
        "seed": 42,   # 尽量冻结服务端采样（部分厂商忽略，忽略亦无副作用）
    }
    last_err = None
    for attempt in range(retries + 1):
        try:
            req = urllib.request.Request(
                url, data=json.dumps(body).encode("utf-8"),
                headers={"Content-Type": "application/json",
                         "Authorization": f"Bearer {api_key}"})
            with urllib.request.urlopen(req, timeout=180) as resp:
                data = json.loads(resp.read().decode("utf-8"))
            return data["choices"][0]["message"]["content"]
        except (urllib.error.URLError, KeyError, json.JSONDecodeError) as e:
            last_err = e
            time.sleep(2 * (attempt + 1))
    raise RuntimeError(f"LLM 调用失败: {last_err}")


def extract_json(text: str) -> dict:
    """从模型输出中提取 JSON 对象（容忍 markdown 围栏与前后杂文本）。"""
    m = re.search(r"\{.*\}", text, re.DOTALL)
    if not m:
        raise ValueError("输出中无 JSON 对象")
    return json.loads(m.group(0))


def anchor_from_line(case_dir: Path, file: str, line: int) -> str:
    """anchor 缺失时按行号取原文（strip）补齐；越界返回空串。"""
    p = case_dir / file
    if not p.is_file():
        return ""
    lines = p.read_text(encoding="utf-8", errors="replace").splitlines()
    if 1 <= line <= len(lines):
        return lines[line - 1].strip()
    return ""


def locate_anchor_line(case_dir: Path, file: str, anchor: str) -> int | None:
    """anchor（去空白）在源文件中首个匹配行行号；找不到返回 None。
    用于校正模型行号幻觉：anchor 逐字照抄可信，line 以本地定位为准。"""
    p = case_dir / file
    if not p.is_file():
        return None
    needle = "".join(anchor.split())
    if not needle:
        return None
    for i, l in enumerate(
            p.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        if needle in "".join(l.split()):
            return i
    return None


def normalize(case_id: str, track: str, case_dir: Path, raw: dict,
              model: str) -> dict:
    """把模型输出清洗成 schema/findings.schema.json 合规文档。"""
    out_findings = []
    for f in raw.get("findings") or []:
        if not isinstance(f, dict):
            continue
        file = str(f.get("file") or "").strip()
        if not file:
            continue
        item = {"file": file}
        anchor = str(f.get("anchor") or "").strip()
        if not anchor and isinstance(f.get("line"), int):
            anchor = anchor_from_line(case_dir, file, f["line"])
        if anchor:
            item["anchor"] = anchor
        else:
            continue  # schema 要求 file+anchor；两者皆缺则丢弃
        # 行号以本地定位为准（修正模型行号幻觉，eval 的 line±tolerance 兜底才有效）
        loc = locate_anchor_line(case_dir, file, anchor)
        if loc:
            item["line"] = loc
        elif isinstance(f.get("line"), int) and f["line"] >= 1:
            item["line"] = f["line"]
        if f.get("function"):
            item["function"] = str(f["function"])
        sc = f.get("scenario")
        if isinstance(sc, str) and SCENARIO_RE.match(sc):
            item["scenario"] = sc
        # scenario 非法或为 null 时按 schema 允许省略/置 null
        sev = f.get("severity")
        if isinstance(sev, str) and sev in SEVERITIES:
            item["severity"] = sev
        out_findings.append(item)
    return {
        "tool": "llm-single-shot",
        "track": track,
        "case_id": case_id,
        "version": model,
        "findings": out_findings,
    }


def review_case(root: Path, case_id: str, base_url: str, api_key: str,
                model: str) -> dict:
    """评审单个 case：找目录 → 组 prompt（contract 轨注入契约）→ 调用 → 归一。"""
    case_dir = None
    for track in ("contract", "defect"):
        d = root / "cases" / track / case_id
        if d.is_dir():
            case_dir = d
            break
    if not case_dir:
        raise FileNotFoundError(f"找不到 case: {case_id}")
    track = case_dir.parent.name

    contract_section = ""
    cy = case_dir / "contract.yaml"
    if track == "contract" and cy.is_file():
        contract_section = CONTRACT_SECTION.format(
            contract_yaml=cy.read_text(encoding="utf-8"))

    prompt = PROMPT_TMPL.format(contract_section=contract_section,
                                sources=collect_sources(case_dir))
    # 解析失败重试一轮：带错误输出让模型自我修复（c21 曾输出非 JSON）
    content = call_llm(base_url, api_key, model, prompt)
    try:
        raw = extract_json(content)
    except (ValueError, json.JSONDecodeError) as e:
        repair = ("你上一次的输出无法解析为 JSON（"
                  f"{e}）。请只输出一个合法 JSON 对象，无 markdown 围栏、无解释文字。\n"
                  f"上一次输出：\n{content[:4000]}")
        content = call_llm(base_url, api_key, model, repair)
        raw = extract_json(content)  # 修复仍失败则抛出
    return normalize(case_id, track, case_dir, raw, model), raw


def main():
    ap = argparse.ArgumentParser(description="LLM 评审 → 归一化 findings")
    ap.add_argument("--cases", required=True,
                    help="逗号分隔 case id 列表（跨 track 自动定位）")
    ap.add_argument("--out", required=True, help="findings 输出目录（每 case 一 json）")
    ap.add_argument("--base-url", default=os.environ.get(
        "AI_REVIEW_BASE_URL", "https://api.deepseek.com"))
    ap.add_argument("--model", default=os.environ.get(
        "AI_REVIEW_MODEL", "deepseek-chat"))
    args = ap.parse_args()

    api_key = os.environ.get("AI_REVIEW_API_KEY")
    if not api_key:
        print("ERROR: 缺少环境变量 AI_REVIEW_API_KEY", file=sys.stderr)
        sys.exit(2)

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    ok = 0
    for cid in [c.strip() for c in args.cases.split(",") if c.strip()]:
        try:
            doc, raw = review_case(ROOT, cid, args.base_url, api_key, args.model)
        except Exception as e:
            print(f"[FAIL] {cid}: {e}", file=sys.stderr)
            continue
        (out_dir / f"{cid}.json").write_text(
            json.dumps(doc, ensure_ascii=False, indent=2), encoding="utf-8")
        # 旁车文件：模型原始输出（含 flow 证据链/reason），不得喂 eval，
        # 仅供 findings_to_sarif 富集 codeFlows（冻结的归一化 schema 不含这些字段）
        (out_dir / f"{cid}.raw.json").write_text(
            json.dumps(raw, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"[ok] {cid}: {len(doc['findings'])} findings")
        ok += 1
    print(f"完成 {ok} 例 → {out_dir}")


if __name__ == "__main__":
    main()
