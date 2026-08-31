# 基线报告 baseline-v1 · Clang Static Analyzer（单 TU 默认 + 原生 CTU）

> 登记日期：2026-08-29
> 工具：`csa-singletu` / `csa-ctu`（clang 21.1.8，官方 LLVM apt 源 `clang-21` / `clang-tools-21`）
> 评测器：`tools/eval.py`（design §4 两层匹配协议）
> 环境钉死（design §4.1 教训①）：Ubuntu clang 21.1.8 / gcc 13 头文件 / `-std=c11 -Wall`
> 口径：每条 finding 经 `sa/adapters/csa_to_findings.py` 转归一化格式（anchor = 源文件对应行源码，去空白）
> 证据归档：`reports/evidence/csa/`（CI artifact 原样落盘 + 复现路径）

## 1. 结论速览

| 模式 | track | cases | pass | FN | FP | EXTRA | recall | bare_fp | contract_viol |
|---|---|---|---|---|---|---|---|---|---|
| 单 TU | contract | 2 | 0 | 2 | 0 | 0 | 0.0 | 0 | 0 |
| 单 TU | defect | 1 | 0 | 1 | 0 | 0 | 0.0 | 0 | - |
| 原生 CTU | contract | 2 | 0 | 2 | 0 | 0 | 0.0 | 0 | 0 |
| 原生 CTU | defect | 1 | 0 | 1 | 0 | 0 | 0.0 | 0 | - |

**两种模式下 recall 均为 0**：三个 must_find 真实缺陷（c01 cwe-190 回绕、c08 cwe-125 跨文件越界读、r10 cwe-125 奇数长度越界读）**全部漏报**。FP 侧干净（未误报任何 must_not_find）。

## 2. 逐例实况

- **c01-upstream-nullguard**（contract / cwe-190）：`guti_group_size` 用 `uint8_t` 累加整组 IE 尺寸，20 个 len=15 累计 340 截断为 84，调用方按回绕值分配缓冲越界写。CSA 未建模整数回绕出界，漏报。
- **c08-protocol-offset-parse**（contract / cwe-125）：`u2u_payload_type_peek` 只查 `size < 12` 就直调无检查访问器，IMSI 长度字段取自报文（可达 255），payload 偏移远超 size 越界读。单 TU 看不到跨文件定义；**原生 CTU 跑通（externalDefMap 生成成功）后仍漏报**——证明此缺陷属 SA 语义盲区，非跨文件信息缺失问题。
- **r10-odd-length-bcd**（defect / cwe-125）：`digits` 为恰好 n 字节定长报文字段（非 NUL 结尾），n 为奇数时末轮 `digits[i+1]` 越界读 1 字节。CSA 把 `i < n; i += 2` 当正常边界，未识别"定长非字符串"形态，漏报。

## 3. 这回答了一个设计疑问

疑问原型："跨文件 case 不是能轻松被 CSA 查出来吗，还用得着 Agent Reviewer？"

实测证伪该前提：
1. **单 TU 默认 CSA 对三类缺陷全漏**（含单文件 r10）。
2. **原生 CTU（clang-extdef-mapping 真实生效）下 c08 仍漏**——CI 日志 `[ok] externalDefMap 生成: 7 行` 证明跨 TU 定义导入已生效，但 c08 的 must_find 依旧查不出。c08 类"旁路入口绕过统一校验 + 不可信字段值驱动偏移"属 SA 语义盲区，CTU 解决不了。
3. 因此 bench 的契约轨价值不在"SA vs Agent 擂台"，而在**统一量分**：SA 在 must_not_find（FP 抑制）与语义型 must_find（TP 检测）两个维度上的弱项，正是 Agent/LLM 评审的增量空间。评分器对所有工具统一出四态，直接可比。

## 4. CTU 工具链补全说明（已解决）

本机自装 clang 21 缺 `clang-extdef-mapping`（初装时仅拷 clang/clang++/clangd），无法本地跑原生 CTU。
补全路径改在 CI：GitHub Actions `ubuntu-latest` 经官方 LLVM apt 源装 `clang-tools-21`，自带 `clang-extdef-mapping-21`，
`sa/runners/run_csa.sh ctu` 走标准流程（`clang-extdef-mapping gen` → `clang --analyze -ctu-dir=... -analyzer-config experimental-enable-naive-ctu=true`）。
CI run 33258703246 已验证 CTU 真生效且 recall 仍 0。

## 5. 复现命令

```bash
# 本地（需 clang-21 + clang-extdef-mapping-21 在 PATH）
./sa/runners/run_csa.sh singletu /tmp/csa_singletu   # 单 TU 默认
./sa/runners/run_csa.sh ctu      /tmp/csa_ctu        # 原生 CTU
python3 tools/eval.py run /tmp/csa_singletu
python3 tools/eval.py run /tmp/csa_ctu
```
或推送后由 `.github/workflows/ci.yml` 自动跑（产物见 reports/evidence/csa/）。
