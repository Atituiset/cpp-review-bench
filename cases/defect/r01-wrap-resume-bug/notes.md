# r01-wrap-resume-bug 用例说明（三段式）

## 1. 真实仓形态（这个 case 从哪来）
RLC/PDCP 发送窗口维护 `tx_next`（模 256），超时恢复时基于 `(tx_next - ack)` 算重传
起点。当 ack 与 tx_next 跨回绕边界时，无符号差值回绕，恢复点错乱（漏发或越界）。

## 2. 真缺陷在哪 / 安全点为什么安全
- **真缺陷**：`resume_point` 的 `(uint8_t)(t->tx_next - t->ack)` 回绕边界处 delta 错乱，
  导致 `win[resume]` 恢复点错位/越界（cwe-190）。
- **安全点**：`rlc_fill` 的 `win[sn]` 写入前 `if (sn < AMAP_MOD)` 受界——安全，非缺陷。

## 3. 各工具可能误判方式
- CSA / CppCheck / clang-tidy：对跨函数 uint8_t 回绕+越界可能漏报（符号/回绕不敏感）。
- cooddy：符号执行+约束求解应能识别回绕边界选错点——对照其相对传统 SA 的精度。
