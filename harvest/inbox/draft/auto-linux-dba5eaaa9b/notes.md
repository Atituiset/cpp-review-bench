# auto-linux-dba5eaaa9b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#22430ae5d90ab288b0ee2ad99ae941f4a666b694](https://github.com/torvalds/linux/commit/22430ae5d90ab288b0ee2ad99ae941f4a666b694) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #22430ae5d90ab288b0ee2ad99ae941f4a666b694 (https://github.com/torvalds/linux/commit/22430ae5d90ab288b0ee2ad99ae941f4a666b694)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 3421；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 22430ae5d90ab288b0ee2ad99ae941f4a666b694 Merge tag 'printk-for-7.3-rc7' of git://git.kernel.org/pub/scm/linux/kernel/git/printk/linux :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3417,8 +3417,11 @@ static void __serial8250_console_write(struct uart_8250_port *up,
 	 * If the console printer did not fully output the previous line, it
 	 * must have been handed or taken over. Insert a newline in order to
 	 * maintain clean output.
+	 *
+	 * Braille consoles are an exception. The serial port is not used
+	 * for printk(). The driver is supposed to write exactly what it gets.
 	 */
-	if (!up->console_line_ended) {
+	if (unlikely(!up->console_line_ended && !nbcon_write_context_is_braille(wctxt))) {
 		if (use_fifo)
 			__serial8250_console_fifo_write(up, wctxt, "\n", 1);
 		else
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`nbcon_enter_unsafe`
- 外部函数：`nbcon_exit_unsafe`
- 外部函数：`serial_lsr_in`
- 外部函数：`serial_port_out`
- 外部函数：`touch_nmi_watchdog`
- 外部函数：`udelay`
- 外部函数：`up_to_u8250p`
- 大写宏：`FIFO`
- 大写宏：`NSEC_PER_USEC`
- 大写宏：`UART_LSR_BOTH_EMPTY`
- 大写宏：`UART_LSR_THRE`
- 大写宏：`UART_TX`
- 外部类型：`Allow`
- 外部类型：`Fallback`
- 外部类型：`Fill`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`Insert`
- 外部类型：`Otherwise`
- 外部类型：`The`
- 外部类型：`Wait`
- 外部类型：`nbcon_write_context`
- 外部类型：`uart_8250_port`
- 外部类型：`uart_port`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-dba5eaaa9b` → 本草稿移入 `cases/defect/auto-linux-dba5eaaa9b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
