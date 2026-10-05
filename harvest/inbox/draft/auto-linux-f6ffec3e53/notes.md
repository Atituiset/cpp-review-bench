# auto-linux-f6ffec3e53

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7704c4c5bb127673b4f0ead839919db573559e38](https://github.com/torvalds/linux/commit/7704c4c5bb127673b4f0ead839919db573559e38) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 47 |
| 编译错误数（gcc syntax-only） | 37（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7704c4c5bb127673b4f0ead839919db573559e38 (https://github.com/torvalds/linux/commit/7704c4c5bb127673b4f0ead839919db573559e38)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 543；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7704c4c5bb127673b4f0ead839919db573559e38 Merge tag 'i2c-fixes-7.3-rc6' of git://git.kernel.org/pub/scm/linux/kernel/git/andi.shyti/linux :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -73,6 +73,9 @@ enum i2c_scl_freq {
  * @prev_msg_tx: Previous message is Tx
  * @quirks: To hold platform specific bug info
  * @smbus_block_read: Flag to handle block read
+ * @smbus_actual_len: Valid byte count (length + payload + optional PEC) of a
+ *	padded SMBus block read. msg->len is trimmed to this on completion.
+ *	Zero when no trimming is needed.
  * @input_clk: Input clock to I2C controller
  * @i2c_clk: I2C SCL frequency
  * @atomic: Mode of transfer
@@ -98,6 +101,7 @@ struct xiic_i2c {
 	bool prev_msg_tx;
 	u32 quirks;
 	bool smbus_block_read;
+	unsigned int smbus_actual_len;
 	unsigned long input_clk;
 	unsigned int i2c_clk;
 	bool atomic;
@@ -539,30 +543,38 @@ static void xiic_smbus_block_read_setup(struct xiic_i2c *i2c)
 
 	/* Check if received length is valid */
 	if (rxmsg_len <= I2C_SMBUS_BLOCK_MAX) {
+		unsigned int pec_len = i2c->rx_msg->len - 1;
+
 		/* Set Receive fifo depth */
-		if (rxmsg_len > IIC_RX_FIFO_DEPTH) {
+		if (rxmsg_len + pec_len > IIC_RX_FIFO_DEPTH) {
 			/*
-			 * When Rx msg len greater than or equal to Rx fifo capacity
-			 * Receive fifo depth should set to Rx fifo capacity minus 1
+			 * Trailing bytes (payload plus any PEC) exceed Rx FIFO
+			 * capacity, so drain in chunks. This also keeps the
+			 * else branch below from pushing rfd_set past the
+			 * 4-bit XIIC_RFD_REG_OFFSET field.
 			 */
 			rfd_set = IIC_RX_FIFO_DEPTH - 1;
-			i2c->rx_msg->len = rxmsg_len + 1;
-		} else if ((rxmsg_len == 1) ||
-			(rxmsg_len == 0)) {
+			i2c->rx_msg->len = rxmsg_len + 1 + pec_len;
+		} else if (1 + rxmsg_len + pec_len < SMBUS_BLOCK_READ_MIN_LEN) {
 			/*
-			 * Minimum of 3 bytes required to exit cleanly. 1 byte
-			 * already received, Second byte is being received. Have
-			 * to set NACK in read_rx before receiving the last byte
+			 * The HW needs SMBUS_BLOCK_READ_MIN_LEN bytes on the
+			 * bus to exit cleanly: by the time the ISR reads the
+			 * length byte the second byte is already being clocked
+			 * in, too late to NACK. Pad the drain target and record
+			 * the real length, trimmed back on completion so the
+			 * PEC check sees the right byte.
 			 */
 			rfd_set = 0;
 			i2c->rx_msg->len = SMBUS_BLOCK_READ_MIN_LEN;
+			i2c->smbus_actual_len = 1 + rxmsg_len + pec_len;
 		} else {
 			/*
-			 * When Rx msg len less than Rx fifo capacity
-			 * Receive fifo depth should set to Rx msg len minus 2
+			 * All trailing bytes fit in the Rx FIFO. Defer RX_FULL
+			 * until every one of them is buffered, so the drain
+			 * takes xiic_read_rx()'s bytes_rem == 0 path.
 			 */
-			rfd_set = rxmsg_len - 2;
-			i2c->rx_msg->len = rxmsg_len + 1;
+			rfd_set = rxmsg_len + pec_len - 1;
+			i2c->rx_msg->len = rxmsg_len + 1 + pec_len;
 		}
 		xiic_setreg8(i2c, XIIC_RFD_REG_OFFSET, rfd_set);
 
@@ -575,6 +587,16 @@ static void xiic_smbus_block_read_setup(struct xiic_i2c *i2c)
 	dev_err(i2c->adap.dev.parent, "smbus_block_read Invalid msg length\n");
 }
 
+/*
+ * Undo the setup-time padding of a short SMBus block read before rx_msg is
+ * cleared, so the PEC check sees the right byte.
+ */
+static void xiic_smbus_trim_len(struct xiic_i2c *i2c)
+{
+	if (i2c->rx_msg && i2c->smbus_actual_len)
+		i2c->rx_msg->len = i2c->smbus_actual_len;
+}
+
 static void xiic_read_rx(struct xiic_i2c *i2c)
 {
 	u8 bytes_in_fifo, cr = 0, bytes_to_read = 0;
@@ -797,6 +819,8 @@ static irqreturn_t xiic_process(int irq, void *dev_id)
 
 		xiic_read_rx(i2c);
 		if (xiic_rx_space(i2c) == 0) {
+			xiic_smbus_trim_len(i2c);
+
 			/* this is the last part of the message */
 			i2c->rx_msg = NULL;
 
@@ -865,8 +889,11 @@ static irqreturn_t xiic_process(int irq, void *dev_id)
 
 		if (i2c->tx_msg && i2c->smbus_block_read) {
 			i2c->smbus_block_read = false;
-			/* Set requested message len=1 to indicate STATE_DONE */
-			i2c->tx_msg->len = 1;
+			/*
+			 * Drive xiic_tx_space() to 0 to signal STATE_DONE
+			 * without truncating the rx_msg length.
+			 */
+			i2c->tx_pos = i2c->tx_msg->len;
 		}
 
 		if (!i2c->tx_msg)
@@ -938
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_dbg`
- 外部函数：`dev_err`
- 外部函数：`ioread32`
- 外部函数：`ioread32be`
- 外部函数：`iowrite32`
- 外部函数：`iowrite32be`
- 外部函数：`iowrite8`
- 外部函数：`xiic_setreg8`
- 大写宏：`BIG`
- 大写宏：`FIFO`
- 大写宏：`I2C`
- 大写宏：`I2C_SMBUS_BLOCK_MAX`
- 大写宏：`ISR`
- 大写宏：`LITTLE`
- 大写宏：`NACK`
- 大写宏：`NULL`
- 大写宏：`SCL`
- 大写宏：`STATE_DONE`
- 大写宏：`STATE_ERROR`
- 大写宏：`STATE_START`
- 外部类型：`Check`
- 外部类型：`Depth`
- 外部类型：`Enable`
- 外部类型：`Flag`
- 外部类型：`Have`
- 外部类型：`Input`
- 外部类型：`Interrupt`
- 外部类型：`Invalid`
- 外部类型：`Lock`
- 外部类型：`Minimum`
- 外部类型：`Mode`
- 外部类型：`Previous`
- 外部类型：`Receive`
- 外部类型：`Register`
- 外部类型：`Rx`
- 外部类型：`Second`
- 外部类型：`Set`
- 外部类型：`Status`
- 外部类型：`To`
- 外部类型：`Tx`
- 外部类型：`When`
- 外部类型：`clk`
- 外部类型：`completion`
- 外部类型：`device`
- 外部类型：`i2c_adapter`
- 外部类型：`i2c_msg`
- 外部类型：`mutex`
- 外部类型：`spinlock_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-f6ffec3e53` → 本草稿移入 `cases/defect/auto-linux-f6ffec3e53/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
