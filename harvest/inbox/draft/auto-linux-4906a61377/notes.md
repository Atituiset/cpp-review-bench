# auto-linux-4906a61377

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9a32e0754d637c1d386a533ae36e41c8f4be8b10](https://github.com/torvalds/linux/commit/9a32e0754d637c1d386a533ae36e41c8f4be8b10) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 124 |
| 编译错误数（gcc syntax-only） | 138（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9a32e0754d637c1d386a533ae36e41c8f4be8b10 (https://github.com/torvalds/linux/commit/9a32e0754d637c1d386a533ae36e41c8f4be8b10)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 708；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9a32e0754d637c1d386a533ae36e41c8f4be8b10 Merge tag 'tty-7.3-rc6' of git://git.kernel.org/pub/scm/linux/kernel/git/gregkh/tty :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -171,7 +171,6 @@ static void qcom_geni_serial_cancel_tx_cmd(struct uart_port *uport);
 static int qcom_geni_serial_port_setup(struct uart_port *uport);
 static void qcom_geni_serial_start_tx_fifo(struct uart_port *uport);
 static void qcom_geni_serial_resume_tx(struct uart_port *uport);
-static void qcom_geni_serial_poll_rx_fifo_locked(struct uart_port *uport);
 
 static inline struct qcom_geni_serial_port *to_dev_port(struct uart_port *uport)
 {
@@ -467,6 +466,102 @@ static int qcom_geni_serial_poll_init(struct uart_port *uport)
 #endif
 
 #ifdef CONFIG_SERIAL_QCOM_GENI_CONSOLE
+static void handle_rx_console(struct uart_port *uport, u32 bytes, bool drop)
+{
+	u32 i;
+	unsigned char buf[sizeof(u32)];
+	struct tty_port *tport;
+	struct qcom_geni_serial_port *port = to_dev_port(uport);
+
+	tport = &uport->state->port;
+	for (i = 0; i < bytes; ) {
+		int c;
+		int chunk = min_t(int, bytes - i, BYTES_PER_FIFO_WORD);
+
+		ioread32_rep(uport->membase + SE_GENI_RX_FIFOn, buf, 1);
+		i += chunk;
+		if (drop)
+			continue;
+
+		for (c = 0; c < chunk; c++) {
+			int sysrq;
+
+			uport->icount.rx++;
+			if (port->brk && buf[c] == 0) {
+				port->brk = false;
+				if (uart_handle_break(uport))
+					continue;
+			}
+
+			sysrq = uart_prepare_sysrq_char(uport, buf[c]);
+
+			if (!sysrq)
+				tty_insert_flip_char(tport, buf[c], TTY_NORMAL);
+		}
+	}
+	if (!drop)
+		tty_flip_buffer_push(tport);
+}
+#else
+static void handle_rx_console(struct uart_port *uport, u32 bytes, bool drop)
+{
+
+}
+#endif
+
+static void qcom_geni_serial_handle_rx_fifo(struct uart_port *uport, bool drop)
+{
+	u32 status;
+	u32 word_cnt;
+	u32 last_word_byte_cnt;
+	u32 last_word_partial;
+	u32 total_bytes;
+
+	status = readl(uport->membase +	SE_GENI_RX_FIFO_STATUS);
+	word_cnt = status & RX_FIFO_WC_MSK;
+	last_word_partial = status & RX_LAST;
+	last_word_byte_cnt = (status & RX_LAST_BYTE_VALID_MSK) >>
+						RX_LAST_BYTE_VALID_SHFT;
+
+	if (!word_cnt)
+		return;
+	total_bytes = BYTES_PER_FIFO_WORD * (word_cnt - 1);
+	if (last_word_partial && last_word_byte_cnt)
+		total_bytes += last_word_byte_cnt;
+	else
+		total_bytes += BYTES_PER_FIFO_WORD;
+	handle_rx_console(uport, total_bytes, drop);
+}
+
+#ifdef CONFIG_SERIAL_QCOM_GENI_CONSOLE
+/* Caller holds the UART port lock. */
+static void qcom_geni_serial_poll_rx_fifo_locked(struct uart_port *uport)
+{
+	struct qcom_geni_serial_port *port = to_dev_port(uport);
+	struct tty_port *tport = &uport->state->port;
+	u32 s_irq_status;
+	bool drop_rx = false;
+
+	s_irq_status = readl(uport->membase + SE_GENI_S_IRQ_STATUS);
+	writel(s_irq_status, uport->membase + SE_GENI_S_IRQ_CLEAR);
+
+	if (s_irq_status & S_RX_FIFO_WR_ERR_EN) {
+		uport->icount.overrun++;
+		tty_insert_flip_char(tport, 0, TTY_OVERRUN);
+	}
+
+	if (s_irq_status & (S_GP_IRQ_0_EN | S_GP_IRQ_1_EN)) {
+		if (s_irq_status & S_GP_IRQ_0_EN)
+			uport->icount.parity++;
+		drop_rx = true;
+	} else if (s_irq_status & (S_GP_IRQ_2_EN | S_GP_IRQ_3_EN)) {
+		uport->icount.brk++;
+		port->brk = true;
+	}
+
+	qcom_geni_serial_handle_rx_fifo(uport, drop_rx);
+}
+
 static void qcom_geni_serial_drain_fifo(struct uart_port *uport)
 {
 	struct qcom_geni_serial_port *port = to_dev_port(uport);
@@ -676,47 +771,6 @@ static void qcom_geni_serial_console_device_unlock(struct console *co,
 #endif
 }
 
-static void handle_rx_console(struct uart_port *uport, u32 bytes, bool drop)
-{
-	u32 i;
-	unsigned char buf[sizeof(u32)];
-	struct tty_port *tport;
-	struct qcom_geni_serial_port *port = to_dev_port(uport);
-
-	tport = &uport->state->port;
-	for (i = 0; i < bytes; ) {
-		int c;
-		int chunk = min_t(int, bytes - i, BYTES_PER_FIFO_WORD);
-
-		ioread32_rep(uport->membase + SE_GENI_RX_FIFOn, buf, 1);
-		i += chunk;
-		if (drop)
-			continue;
-
-		for (c = 0; c < chunk; c++) {
-			int sysrq;
-
-			uport->icount.rx++;
-			if (port->brk && buf[c] == 0) {
-				port->brk = false;
-				if (uart_handle_break(uport))
-					continue;
-			}
-
-			sysrq = uart_prepare_sysrq_char(uport, buf[c]);
-
-			if
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__uart_port_unlock_irqrestore`
- 外部函数：`data`
- 外部函数：`dev_err`
- 外部函数：`dev_err_ratelimited`
- 外部函数：`devm_krealloc`
- 外部函数：`devm_kzalloc`
- 外部函数：`geni_icc_set_bw`
- 外部函数：`geni_load_se_firmware`
- 外部函数：`geni_se_config_packing`
- 外部函数：`geni_se_get_rx_fifo_depth`
- 外部函数：`geni_se_get_tx_fifo_depth`
- 外部函数：`geni_se_get_tx_fifo_width`
- 外部函数：`geni_se_init`
- 外部函数：`geni_se_read_proto`
- 外部函数：`geni_se_select_mode`
- 外部函数：`handle_sysrq`
- 外部函数：`ida_alloc_range`
- 外部函数：`int`
- 外部函数：`ioread32_rep`
- 外部函数：`kfifo_is_empty`
- 外部函数：`min_t`
- 外部函数：`of_alias_get_highest_id`
- 外部函数：`pm_runtime_force_resume`
- 外部函数：`pr_err`
- 外部函数：`readl`
- 外部函数：`stop_rx`
- 外部函数：`tty_flip_buffer_push`
- 外部函数：`tty_insert_flip_char`
- 外部函数：`tty_insert_flip_string`
- 外部函数：`uart_console`
- 外部函数：`uart_console_write`
- 外部函数：`uart_fifo_out`
- 外部函数：`uart_handle_break`
- 外部函数：`uart_parse_options`
- 外部函数：`uart_prepare_sysrq_char`
- 外部函数：`uart_resume_port`
- 外部函数：`uart_set_options`
- 外部函数：`uart_tx_stopped`
- 外部函数：`udelay`
- 外部函数：`unlikely`
- 外部函数：`writel`
- 大写宏：`BITS_PER_BYTE`
- 大写宏：`CONFIG_MAGIC_SYSRQ_SERIAL`
- 大写宏：`CONFIG_SERIAL_QCOM_GENI_CONSOLE`
- 大写宏：`CONFIG_SERIAL_QCOM_GENI_UART_PORTS`
- 大写宏：`CON_NBCON`
- 大写宏：`CON_PRINTBUFFER`
- 大写宏：`CORE`
- 大写宏：`CORE_2X_50_MHZ`
- 大写宏：`CPU`
- 大写宏：`CPU_TO_GENI`
- 大写宏：`CTS`
- 大写宏：`DIV_ROUND_UP`
- 大写宏：`ENOMEM`
- 大写宏：`ENXIO`
- 大写宏：`ERR_PTR`
- 大写宏：`FIFO`
- 大写宏：`GENI_DEFAULT_BW`
- 大写宏：`GENI_SE_INVALID_PROTO`
- 大写宏：`GENI_SE_UART`
- 大写宏：`GENI_TO_CORE`
- 大写宏：`GENMASK`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IS_ERR`
- 大写宏：`M_CMD_ABORT_EN`
- 大写宏：`M_CMD_DONE_EN`
- 大写宏：`M_GENI_CMD_ABORT`
- 大写宏：`M_GENI_CMD_ACTIVE`
- 大写宏：`M_OPCODE_SHFT`
- 大写宏：`M_TX_FIFO_WATERMARK_EN`
- 大写宏：`NOTE`
- 大写宏：`NULL`
- 大写宏：`PTR_ERR`
- 大写宏：`RTS`
- 大写宏：`RX_FIFO_WC_MSK`
- 大写宏：`RX_LAST`
- 大写宏：`RX_LAST_BYTE_VALID_MSK`
- 大写宏：`RX_LAST_BYTE_VALID_SHFT`
- 大写宏：`SE_GENI_M_CMD0`
- 大写宏：`SE_GENI_M_CMD_CTRL_REG`
- 大写宏：`SE_GENI_M_IRQ_CLEAR`
- 大写宏：`SE_GENI_M_IRQ_EN`
- 大写宏：`SE_GENI_M_IRQ_STATUS`
- 大写宏：`SE_GENI_RX_FIFO_STATUS`
- 大写宏：`SE_GENI_STATUS`
- 大写宏：`SE_GENI_S_IRQ_CLEAR`
- 大写宏：`SE_GENI_S_IRQ_STATUS`
- 大写宏：`SE_GENI_TX_FIFO_STATUS`
- 大写宏：`SE_GENI_TX_WATERMARK_REG`
- 大写宏：`S_GP_IRQ_0_EN`
- 大写宏：`S_GP_IRQ_1_EN`
- 大写宏：`S_GP_IRQ_2_EN`
- 大写宏：`S_GP_IRQ_3_EN`
- 大写宏：`S_RX_FIFO_WR_ERR_EN`
- 大写宏：`TTY_NORMAL`
- 大写宏：`TTY_OVERRUN`
- 大写宏：`UART`
- 大写宏：`UPF_BOOT_AUTOCONF`
- 大写宏：`UPIO_MEM`
- 外部类型：`Account`
- 外部类型：`Bump`
- 外部类型：`Caller`
- 外部类型：`Configure`
- 外部类型：`If`
- 外部类型：`Invalid`
- 外部类型：`Make`
- 外部类型：`Note`
- 外部类型：`SE_GENI_RX_FIFOn`
- 外部类型：`SE_GENI_TX_FIFOn`
- 外部类型：`Start`
- 外部类型：`SysRq`
- 外部类型：`The`
- 外部类型：`Tx`
- 外部类型：`Unfortunately`
- 外部类型：`Use`
- 外部类型：`console`
- 外部类型：`device`
- 外部类型：`dma_addr_t`
- 外部类型：`earlycon_device`
- 外部类型：`geni_se`
- 外部类型：`geni_se_xfer_mode`
- 外部类型：`irq_work`
- 外部类型：`notifier_block`
- 外部类型：`size_t`
- 外部类型：`tty_port`
- 外部类型：`uart_driver`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-4906a61377` → 本草稿移入 `cases/defect/auto-linux-4906a61377/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
