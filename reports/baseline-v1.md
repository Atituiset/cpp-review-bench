# 基线报告 baseline-v1 · Clang Static Analyzer（单 TU 默认配置）

> 登记日期：2026-08-29
> 工具：`csa-singletu`（clang 21.1.8，`--analyze` 默认 checker，单编译单元逐文件分析）
> 评测器：`tools/eval.py`（design §4 两层匹配协议）
> 环境钉死（design §4.1 教训①）：clang 21.1.8 / gcc 13 头文件 / `-std=c11 -Wall`
> 口径：每条 finding 经 `tools/csa_to_findings.py` 转归一化格式（anchor = 源文件对应行源码，去空白）

## 1. 结论速览

| track | cases | pass | FN | FP | EXTRA | recall | contract_viol | bare_fp |
|---|---|---|---|---|---|---|---|---|
| contract | 2 | 0 | 2 | 0 | 0 | 0.0 | 0 | 0 |
| defect | 1 | 0 | 1 | 0 | 0 | 0.0 | - | 0 |

CSA（单 TU 默认）对本期 3 例 **recall = 0**：三个 must_find 真实缺陷（c01 cwe-190 回绕、c08 cwe-125 跨文件越界读、r10 cwe-125 奇数长度越界读）**全部漏报**。FP 侧干净（未误报任何 must_not_find）。

## 2. 逐例实况

- **c01-upstream-nullguard**（contract / cwe-190）：`guti_group_size` 用 `uint8_t` 累加整组 IE 尺寸，20 个 len=15 累计 340 截断为 84，调用方按回绕值分配缓冲越界写。CSA 未建模整数回绕出界，漏报。
- **c08-protocol-offset-parse**（contract / cwe-125）：`u2u_payload_type_peek` 只查 `size < 12` 就直调无检查访问器，IMSI 长度字段取自报文（可达 255），payload 偏移远超 size 越界读。单 TU 分析看不到 `u2u_payload_offset` 跨文件定义，且未建模"字段值来自不可信输入"，漏报。补充实验：将两文件合并为单 TU（等价于 CTU 上限近似）后 CSA 仍漏报此 must_find，仅报一条无关的 `core.uninitialized.UndefReturn` 噪声。
- **r10-odd-length-bcd**（defect / cwe-125）：`digits` 为恰好 n 字节定长报文字段（非 NUL 结尾），n 为奇数时末轮 `digits[i+1]` 越界读 1 字节。CSA 把 `i < n; i += 2` 当正常边界，未识别"定长非字符串"形态，漏报。

## 3. 这回答了一个设计疑问

疑问原型："跨文件 case 不是能轻松被 CSA 查出来吗，还用得着 Agent Reviewer？"

实测证伪该前提：
1. **单 TU 默认 CSA 对三类缺陷全漏**（含单文件 r10）。
2. **合并 TU（CTU 上限近似）下 c08 仍漏**——证明 c08 类"旁路入口绕过统一校验 + 不可信字段值驱动偏移"属 SA 语义盲区，非开关问题。
3. 因此 bench 的契约轨价值不在"SA vs Agent 擂台"，而在**统一量分**：SA 在 must_not_find（FP 抑制）与语义型 must_find（TP 检测）两个维度上的弱项，正是 Agent/LLM 评审的增量空间。评分器对所有工具统一出四态，直接可比。

## 4. 待补实验（CTU 全火力）

本机 clang 21 缺 `clang-extdef-mapping`（原生 CTU 必需工具，官方预编译 tarball 的 bin/ 内含，初装时仅拷了 clang/clang++/clangd）。CTU 列（`csa-ctu`）待补齐工具链后补登：
- 计划：下载 LLVM 21.1.8 官方 tarball → 抽出 `clang-extdef-mapping` + 补充 `lib/` → `clang-extdef-mapping gen` 生成 externalDefMap → 逐 TU 生成 CTU 索引 → `clang --analyze -ctu-dir=...` 重跑 c08。
- 预期：CTU 仅做部分定义导入，弱于"合并 TU"近似；c08 的 must_find 被查出的可能性低（与 §2 合并 TU 结论一致）。无论 CTU 结果如何，本节 recall=0 的结论稳健。

## 5. 复现命令

```
INC="-isystem /usr/lib/gcc/x86_64-linux-gnu/13/include -isystem /usr/include/x86_64-linux-gnu -isystem /usr/include"
for c in cases/contract/c01-upstream-nullguard/src/*.c cases/contract/c08-protocol-offset-parse/src/*.c cases/defect/r10-odd-length-bcd/src/*.c; do
  clang --analyze -Xanalyzer -analyzer-output=plist -o /tmp/csa/$(basename $c).plist -std=c11 $INC $c
done
# 逐 case 用 tools/csa_to_findings.py 转归一化 findings，再 tools/eval.py run <dir>
```
