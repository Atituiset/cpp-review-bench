# auto-linux-0824216b1a

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
| 外部依赖数（dep_count） | 266 |
| 编译错误数（gcc syntax-only） | 180（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9a32e0754d637c1d386a533ae36e41c8f4be8b10 (https://github.com/torvalds/linux/commit/9a32e0754d637c1d386a533ae36e41c8f4be8b10)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 4178；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9a32e0754d637c1d386a533ae36e41c8f4be8b10 Merge tag 'tty-7.3-rc6' of git://git.kernel.org/pub/scm/linux/kernel/git/gregkh/tty :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -454,14 +454,13 @@ static const u8 gsm_fcs8[256] = {
 
 static void gsm_dlci_close(struct gsm_dlci *dlci);
 static int gsmld_output(struct gsm_mux *gsm, u8 *data, int len);
-static int gsm_modem_update(struct gsm_dlci *dlci, u8 brk);
+static int gsm_modem_update(struct gsm_dlci *dlci, u8 brk, bool wait);
 static struct gsm_msg *gsm_data_alloc(struct gsm_mux *gsm, u8 addr, int len,
 								u8 ctrl);
 static int gsm_send_packet(struct gsm_mux *gsm, struct gsm_msg *msg);
 static struct gsm_dlci *gsm_dlci_alloc(struct gsm_mux *gsm, int addr);
 static void gsmld_write_trigger(struct gsm_mux *gsm);
 static void gsmld_write_task(struct work_struct *work);
-static int gsm_modem_send_initial_msc(struct gsm_dlci *dlci);
 
 /**
  *	gsm_fcs_add	-	update FCS
@@ -2174,7 +2173,7 @@ static void gsm_dlci_open(struct gsm_dlci *dlci)
 		pr_debug("DLCI %d goes open.\n", dlci->addr);
 	/* Send current modem state */
 	if (dlci->addr) {
-		gsm_modem_send_initial_msc(dlci);
+		gsm_modem_update(dlci, 0, false);
 	} else {
 		/* Start keep-alive control */
 		gsm->ka_num = 0;
@@ -4136,9 +4135,10 @@ static void gsm_modem_upd_via_data(struct gsm_dlci *dlci, u8 brk)
  *	gsm_modem_upd_via_msc	-	send modem bits via control frame
  *	@dlci: channel
  *	@brk: break signal
+ *	@wait: wait for the MSC command
  */
 
-static int gsm_modem_upd_via_msc(struct gsm_dlci *dlci, u8 brk)
+static int gsm_modem_upd_via_msc(struct gsm_dlci *dlci, u8 brk, bool wait)
 {
 	u8 modembits[3];
 	struct gsm_control *ctrl;
@@ -4155,41 +4155,22 @@ static int gsm_modem_upd_via_msc(struct gsm_dlci *dlci, u8 brk)
 		modembits[2] = (brk << 4) | 2 | EA; /* Length, Break, EA */
 		len++;
 	}
+	if (!wait)
+		return gsm_control_command(dlci->gsm, CMD_MSC, modembits, len);
 	ctrl = gsm_control_send(dlci->gsm, CMD_MSC, modembits, len);
 	if (ctrl == NULL)
 		return -ENOMEM;
 	return gsm_control_wait(dlci->gsm, ctrl);
 }
 
-/**
- * gsm_modem_send_initial_msc - Send initial modem status message
- *
- * @dlci: channel
- *
- * Send an initial MSC message after DLCI open to set the initial
- * modem status lines. This is only done for basic mode.
- * Does not wait for a response as we cannot block the input queue
- * processing.
- */
-static int gsm_modem_send_initial_msc(struct gsm_dlci *dlci)
-{
-	u8 modembits[2];
-
-	if (dlci->adaption != 1 || dlci->gsm->encoding != GSM_BASIC_OPT)
-		return 0;
-
-	modembits[0] = (dlci->addr << 2) | 2 | EA; /* DLCI, Valid, EA */
-	modembits[1] = (gsm_encode_modem(dlci) << 1) | EA;
-	return gsm_control_command(dlci->gsm, CMD_MSC, (const u8 *)&modembits, 2);
-}
-
 /**
  *	gsm_modem_update	-	send modem status line state
  *	@dlci: channel
  *	@brk: break signal
+ *	@wait: wait for the MSC command
  */
 
-static int gsm_modem_update(struct gsm_dlci *dlci, u8 brk)
+static int gsm_modem_update(struct gsm_dlci *dlci, u8 brk, bool wait)
 {
 	if (dlci->gsm->dead)
 		return -EL2HLT;
@@ -4199,7 +4180,7 @@ static int gsm_modem_update(struct gsm_dlci *dlci, u8 brk)
 		return 0;
 	} else if (dlci->gsm->encoding == GSM_BASIC_OPT) {
 		/* Send as MSC control message. */
-		return gsm_modem_upd_via_msc(dlci, brk);
+		return gsm_modem_upd_via_msc(dlci, brk, wait);
 	}
 
 	/* Modem status lines are not supported. */
@@ -4265,7 +4246,7 @@ static void gsm_dtr_rts(struct tty_port *port, bool active)
 		modem_tx &= ~(TIOCM_DTR | TIOCM_RTS);
 	if (modem_tx != dlci->modem_tx) {
 		dlci->modem_tx = modem_tx;
-		gsm_modem_update(dlci, 0);
+		gsm_modem_update(dlci, 0, true);
 	}
 }
 
@@ -4471,7 +4452,7 @@ static int gsmtty_tiocmset(struct tty_struct *tty,
 
 	if (modem_tx != dlci->modem_tx) {
 		dlci->modem_tx = modem_tx;
-		return gsm_modem_update(dlci, 0);
+		return gsm_modem_update(dlci, 0, true);
 	}
 	return 0;
 }
@@ -4553,7 +4534,7 @@ static void gsmtty_throttle(struct tty_struct *tty)
 		dlci->modem_tx &= ~TIOCM_RTS;
 	dlci->throttled = true;
 	/* Send an MSC with RTS cleared */
-	gsm_modem_update(dlci, 0);
+	gsm_modem_update(dlci, 0, true);
 }
 
 static void gsmtty_unthrottle
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`block`
- 外部函数：`clear_bit`
- 外部函数：`container_of`
- 外部函数：`cpu_to_le16`
- 外部函数：`dev_kfree_skb_any`
- 外部函数：`framed`
- 外部函数：`get_unaligned_le16`
- 外部函数：`gsmtty_open`
- 外部函数：`kasprintf`
- 外部函数：`kfifo_alloc`
- 外部函数：`kfifo_len`
- 外部函数：`kfifo_out_locked`
- 外部函数：`kfifo_reset`
- 外部函数：`kfree`
- 外部函数：`kmalloc`
- 外部函数：`kref_put`
- 外部函数：`kzalloc_obj`
- 外部函数：`list_add_tail`
- 外部函数：`list_del`
- 外部函数：`list_empty`
- 外部函数：`memcpy`
- 外部函数：`mod_timer`
- 外部函数：`mutex_destroy`
- 外部函数：`mutex_init`
- 外部函数：`octet`
- 外部函数：`octets`
- 外部函数：`pr_cont`
- 外部函数：`pr_debug`
- 外部函数：`pr_err`
- 外部函数：`pr_info`
- 外部函数：`print_hex_dump`
- 外部函数：`requested`
- 外部函数：`roundup`
- 外部函数：`schedule_work`
- 外部函数：`set_bit`
- 外部函数：`skb_dequeue_tail`
- 外部函数：`skb_pull`
- 外部函数：`skb_queue_head_init`
- 外部函数：`skb_queue_tail`
- 外部函数：`spin_lock_init`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`spin_unlock_irqrestore`
- 外部函数：`timer_container_of`
- 外部函数：`timer_delete`
- 外部函数：`timer_setup`
- 外部函数：`tty_flip_buffer_push`
- 外部函数：`tty_hangup`
- 外部函数：`tty_insert_flip_char`
- 外部函数：`tty_insert_flip_string`
- 外部函数：`tty_kref_put`
- 外部函数：`tty_port_init`
- 外部函数：`tty_port_put`
- 外部函数：`tty_port_set_initialized`
- 外部函数：`tty_port_tty_get`
- 外部函数：`tty_port_tty_hangup`
- 外部函数：`tty_port_tty_wakeup`
- 外部函数：`tty_wakeup`
- 外部函数：`tty_write_room`
- 外部函数：`void`
- 外部函数：`wait_event`
- 外部函数：`wake_up`
- 外部函数：`wake_up_all`
- 外部函数：`wake_up_interruptible`
- 外部函数：`write`
- 大写宏：`ADDR`
- 大写宏：`ADM`
- 大写宏：`BIT`
- 大写宏：`CTRL`
- 大写宏：`C_CLOCAL`
- 大写宏：`C_CRTSCTS`
- 大写宏：`DATA`
- 大写宏：`DLCI`
- 大写宏：`DLCI0`
- 大写宏：`DLCI_CLOSED`
- 大写宏：`DLCI_CLOSING`
- 大写宏：`DLCI_CONFIGURE`
- 大写宏：`DLCI_MODE_ABM`
- 大写宏：`DLCI_MODE_ADM`
- 大写宏：`DLCI_OPEN`
- 大写宏：`DLCI_OPENING`
- 大写宏：`DLCI_WAITING_CONFIG`
- 大写宏：`DUMP_PREFIX_NONE`
- 大写宏：`DUMP_PREFIX_OFFSET`
- 大写宏：`EAGAIN`
- 大写宏：`EINVAL`
- 大写宏：`EL2HLT`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`ENOSPC`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`FCS`
- 大写宏：`FIELD_GET`
- 大写宏：`FIELD_PREP`
- 大写宏：`FIXME`
- 大写宏：`GENMASK`
- 大写宏：`GFP_ATOMIC`
- 大写宏：`GFP_KERNEL`
- 大写宏：`GSM`
- 大写宏：`GSM0_ADDRESS`
- 大写宏：`GSM0_CONTROL`
- 大写宏：`GSM0_DATA`
- 大写宏：`GSM0_FCS`
- 大写宏：`GSM0_LEN0`
- 大写宏：`GSM0_LEN1`
- 大写宏：`GSM0_SSOF`
- 大写宏：`GSM1_ADDRESS`
- 大写宏：`GSM1_CONTROL`
- 大写宏：`GSM1_DATA`
- 大写宏：`GSM1_OVERRUN`
- 大写宏：`GSM1_START`
- 大写宏：`GSM_ADV_OPT`
- 大写宏：`GSM_BASIC_OPT`
- 大写宏：`GSM_SEARCH`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`KERN_INFO`
- 大写宏：`LEN`
- 大写宏：`MAX_MTU`
- 大写宏：`MIN_MTU`
- 大写宏：`MRU`
- 大写宏：`MSC`
- 大写宏：`MTU`
- 大写宏：`MUX`
- 大写宏：`NSC`
- 大写宏：`NULL`
- 大写宏：`RTS`
- 大写宏：`SOF`
- 大写宏：`STFU`
- 大写宏：`TEST`
- 大写宏：`TIOCM_CD`
- 大写宏：`TIOCM_CTS`
- 大写宏：`TIOCM_DSR`
- 大写宏：`TIOCM_DTR`
- 大写宏：`TIOCM_OUT1`
- 大写宏：`TIOCM_OUT2`
- 大写宏：`TIOCM_RI`
- 大写宏：`TIOCM_RTS`
- 大写宏：`TTY_BREAK`
- 大写宏：`TTY_DO_WRITE_WAKEUP`
- 大写宏：`TTY_FRAME`
- 大写宏：`TTY_OVERRUN`
- 大写宏：`TTY_PARITY`
- 大写宏：`WARN_ON`
- 外部类型：`Adaption`
- 外部类型：`Add`
- 外部类型：`All`
- 外部类型：`Allow`
- 外部类型：`Always`
- 外部类型：`Asked`
- 外部类型：`Asynchronous`
- 外部类型：`Balanced`
- 外部类型：`Best`
- 外部类型：`Bits`
- 外部类型：`Break`
- 外部类型：`Bytes`
- 外部类型：`Can`
- 外部类型：`Carrier`
- 外部类型：`Clear`
- 外部类型：`Close`
- 外部类型：`Closed`
- 外部类型：`Command`
- 外部类型：`Configuration`
- 外部类型：`Control`
- 外部类型：`DLCIs`
- 外部类型：`DLCs`
- 外部类型：`Data`
- 外部类型：`Details`
- 外部类型：`Devices`
- 外部类型：`Did`
- 外部类型：`Disconnected`
- 外部类型：`Does`
- 外部类型：`Done`
- 外部类型：`Ensure`
- 外部类型：`Error`
- 外部类型：`Events`
- 外部类型：`Fill`
- 外部类型：`Flag`
- 外部类型：`Flow`
- 外部类型：`Frame`
- 外部类型：`Framing`
- 外部类型：`Has`
- 外部类型：`If`
- 外部类型：`Interruptible`
- 外部类型：`Invalid`
- 外部类型：`Keep`
- 外部类型：`Kick`
- 外部类型：`Layer`
- 外部类型：`Length`
- 外部类型：`Limits`
- 外部类型：`Line`
- 外部类型：`Link`
- 外部类型：`Malformed`
- 外部类型：`Map`
- 外部类型：`Method`
- 外部类型：`Mode`
- 外部类型：`Modem`
- 外部类型：`Move`
- 外部类型：`Must`
- 外部类型：`Need`
- 外部类型：`Negotiation`
- 外部类型：`No`
- 外部类型：`Normal`
- 外部类型：`Note`
- 外部类型：`Notify`
- 外部类型：`Off`
- 外部类型：`On`
- 外部类型：`One`
- 外部类型：`Only`
- 外部类型：`Optional`
- 外部类型：`Or`
- 外部类型：`Our`
- 外部类型：`Out`
- 外部类型：`Over`
- 外部类型：`Packetised`
- 外部类型：`Pending`
- 外部类型：`Points`
- 外部类型：`Power`
- 外部类型：`Prevent`
- 外部类型：`Priority`
- 外部类型：`Private`
- 外部类型：`Proceed`
- 外部类型：`Process`
- 外部类型：`Propose`
- 外部类型：`Protects`
- 外部类型：`Queue`
- 外部类型：`Queued`
- 外部类型：`Refuse`
- 外部类型：`Rejected`
- 外部类型：`Remote`
- 外部类型：`Reply`
- 外部类型：`Retransmit`
- 外部类型：`Retry`
- 外部类型：`See`
- 外部类型：`Send`
- 外部类型：`Sending`
- 外部类型：`Serialize`
- 外部类型：`Service`
- 外部类型：`Set`
- 外部类型：`Skip`
- 外部类型：`Start`
- 外部类型：`TTYs`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Throttle`
- 外部类型：`Timers`
- 外部类型：`Too`
- 外部类型：`Transmission`
- 外部类型：`Ugly`
- 外部类型：`Unstructured`
- 外部类型：`Unsupported`
- 外部类型：`Uplink`
- 外部类型：`Valid`
- 外部类型：`Wait`
- 外部类型：`Waiting`
- 外部类型：`Was`
- 外部类型：`We`
- 外部类型：`Window`
- 外部类型：`kfifo`
- 外部类型：`kref`
- 外部类型：`list_head`
- 外部类型：`mutex`
- 外部类型：`net_device`
- 外部类型：`sk_buff`
- 外部类型：`sk_buff_head`
- 外部类型：`spinlock_t`
- 外部类型：`timer_list`
- 外部类型：`tty_port`
- 外部类型：`tty_struct`
- 外部类型：`wait_queue_head_t`
- 外部类型：`work_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-0824216b1a` → 本草稿移入 `cases/defect/auto-linux-0824216b1a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
