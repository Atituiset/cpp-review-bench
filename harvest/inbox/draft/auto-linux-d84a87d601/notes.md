# auto-linux-d84a87d601

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
| 外部依赖数（dep_count） | 40 |
| 编译错误数（gcc syntax-only） | 29（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9a32e0754d637c1d386a533ae36e41c8f4be8b10 (https://github.com/torvalds/linux/commit/9a32e0754d637c1d386a533ae36e41c8f4be8b10)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 673；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9a32e0754d637c1d386a533ae36e41c8f4be8b10 Merge tag 'tty-7.3-rc6' of git://git.kernel.org/pub/scm/linux/kernel/git/gregkh/tty :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -216,6 +216,8 @@
 #define SC16IS7XX_TLR_TX_TRIGGER(words)	((((words) / 4) & 0x0f) << 0)
 #define SC16IS7XX_TLR_RX_TRIGGER(words)	((((words) / 4) & 0x0f) << 4)
 
+#define SC16IS7XX_TX_TRIGGER_LEVEL	32
+
 /* IOControl register bits (Only 75x/76x) */
 #define SC16IS7XX_IOCONTROL_LATCH_BIT	BIT(0)   /* Enable input latching */
 #define SC16IS7XX_IOCONTROL_MODEM_A_BIT	BIT(1)   /* Enable GPIO[7:4] as modem A pins */
@@ -647,12 +649,26 @@ static void sc16is7xx_handle_rx(struct uart_port *port, unsigned int rxlen,
 	tty_flip_buffer_push(&port->state->port);
 }
 
+static unsigned int sc16is7xx_txlvl(struct uart_port *port)
+{
+	unsigned int txlvl;
+
+	txlvl = sc16is7xx_port_read(port, SC16IS7XX_TXLVL_REG);
+	if (txlvl > SC16IS7XX_FIFO_SIZE) {
+		dev_err_ratelimited(port->dev,
+				    "chip reports %u free bytes in TX FIFO, but it only has %u\n",
+				    txlvl, SC16IS7XX_FIFO_SIZE);
+		return 0;
+	}
+
+	return txlvl;
+}
+
 static void sc16is7xx_handle_tx(struct uart_port *port)
 {
 	struct tty_port *tport = &port->state->port;
 	unsigned long flags;
 	unsigned int txlen;
-	unsigned char *tail;
 
 	if (unlikely(port->x_char)) {
 		sc16is7xx_port_write(port, SC16IS7XX_THR_REG, port->x_char);
@@ -669,17 +685,28 @@ static void sc16is7xx_handle_tx(struct uart_port *port)
 	}
 
 	/* Limit to space available in TX FIFO */
-	txlen = sc16is7xx_port_read(port, SC16IS7XX_TXLVL_REG);
-	if (txlen > SC16IS7XX_FIFO_SIZE) {
-		dev_err_ratelimited(port->dev,
-			"chip reports %d free bytes in TX fifo, but it only has %d",
-			txlen, SC16IS7XX_FIFO_SIZE);
-		txlen = 0;
-	}
+	txlen = sc16is7xx_txlvl(port);
+
+	/* Handle circular buffer wrap-around by sending multiple segments */
+	while (txlen > 0 && !kfifo_is_empty(&tport->xmit_fifo)) {
+		unsigned char *tail;
+		unsigned int to_send;
+
+		to_send = kfifo_out_linear_ptr(&tport->xmit_fifo, &tail, txlen);
+		if (!to_send)
+			break;
 
-	txlen = kfifo_out_linear_ptr(&tport->xmit_fifo, &tail, txlen);
-	sc16is7xx_fifo_write(port, tail, txlen);
-	uart_xmit_advance(port, txlen);
+		sc16is7xx_fifo_write(port, tail, to_send);
+		uart_xmit_advance(port, to_send);
+
+		if (kfifo_is_empty(&tport->xmit_fifo))
+			break;
+
+		/* Refill below the trigger to enable the next THRI crossing. */
+		txlen = sc16is7xx_txlvl(port);
+		if (txlen < SC16IS7XX_TX_TRIGGER_LEVEL)
+			break;
+	}
 
 	uart_port_lock_irqsave(port, &flags);
 	if (kfifo_len(&tport->xmit_fifo) < WAKEUP_CHARS)
@@ -1130,6 +1157,10 @@ static int sc16is7xx_startup(struct uart_port *port)
 			     SC16IS7XX_TCR_RX_RESUME(24) |
 			     SC16IS7XX_TCR_RX_HALT(48));
 
+	/* Sync hardware and software TX trigger levels */
+	sc16is7xx_port_write(port, SC16IS7XX_TLR_REG,
+			     SC16IS7XX_TLR_TX_TRIGGER(SC16IS7XX_TX_TRIGGER_LEVEL));
+
 	/* Disable TCR/TLR access */
 	sc16is7xx_port_update(port, SC16IS7XX_MCR_REG, SC16IS7XX_MCR_TCRTLR_BIT, 0);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bits`
- 外部函数：`container_of`
- 外部函数：`dev_err_ratelimited`
- 外部函数：`kfifo_len`
- 外部函数：`kfifo_out_linear_ptr`
- 外部函数：`regmap_noinc_write`
- 外部函数：`regmap_read`
- 外部函数：`regmap_update_bits`
- 外部函数：`regmap_write`
- 外部函数：`sc16is7xx_port_read`
- 外部函数：`tty_flip_buffer_push`
- 外部函数：`uart_port_lock_irqsave`
- 外部函数：`uart_xmit_advance`
- 外部函数：`unlikely`
- 大写宏：`BIT`
- 大写宏：`EFR`
- 大写宏：`FIFO`
- 大写宏：`GPIO`
- 大写宏：`SPI`
- 大写宏：`TCR`
- 大写宏：`TLR`
- 大写宏：`TXLVL`
- 大写宏：`WAKEUP_CHARS`
- 外部类型：`Control`
- 外部类型：`Disable`
- 外部类型：`Don`
- 外部类型：`Enable`
- 外部类型：`For`
- 外部类型：`IOControl`
- 外部类型：`Limit`
- 外部类型：`Modem`
- 外部类型：`Only`
- 外部类型：`Rx`
- 外部类型：`Value`
- 外部类型：`kthread_delayed_work`
- 外部类型：`kthread_work`
- 外部类型：`mutex`
- 外部类型：`regmap`
- 外部类型：`tty_port`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-d84a87d601` → 本草稿移入 `cases/defect/auto-linux-d84a87d601/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
