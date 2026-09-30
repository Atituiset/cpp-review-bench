# auto-linux-96e485cf7c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a243ede718463c7b481878656f1ff32a0ce0fd54](https://github.com/torvalds/linux/commit/a243ede718463c7b481878656f1ff32a0ce0fd54) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 120 |
| 编译错误数（gcc syntax-only） | 144（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a243ede718463c7b481878656f1ff32a0ce0fd54 (https://github.com/torvalds/linux/commit/a243ede718463c7b481878656f1ff32a0ce0fd54)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1724；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a243ede718463c7b481878656f1ff32a0ce0fd54 Merge tag 'mtd/fixes-for-7.3-rc6' of git://git.kernel.org/pub/scm/linux/kernel/git/mtd/linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1716,12 +1716,24 @@ static int cfi_intelext_write_words (struct mtd_info *mtd, loff_t to , size_t le
 }
 
 
+/*
+ * Keep noinline: inlined, the map_word temporaries put do_write_buffer() over
+ * the frame-size limit with MTD_MAP_BANK_WIDTH_32 and KASAN_STACK.
+ */
+static noinline void __xipram cfi_write_cmd(struct map_info *map,
+					    unsigned long cmd, unsigned long adr)
+{
+	struct cfi_private *cfi = map->fldrv_priv;
+
+	map_write(map, CMD(cmd), adr);
+}
+
 static int __xipram do_write_buffer(struct map_info *map, struct flchip *chip,
 				    unsigned long adr, const struct kvec **pvec,
 				    unsigned long *pvec_seek, int len)
 {
 	struct cfi_private *cfi = map->fldrv_priv;
-	map_word status, write_cmd, datum;
+	map_word status, datum;
 	unsigned long cmd_adr;
 	int ret, wbufsize, word_gap, words;
 	const struct kvec *vec;
@@ -1740,9 +1752,6 @@ static int __xipram do_write_buffer(struct map_info *map, struct flchip *chip,
 	if (is_LH28F640BF(cfi))
 		cmd_adr = adr;
 
-	/* Let's determine this according to the interleave only once */
-	write_cmd = (cfi->cfiq->P_ID != P_ID_INTEL_PERFORMANCE) ? CMD(0xe8) : CMD(0xe9);
-
 	mutex_lock(&chip->mutex);
 	ret = get_chip(map, chip, cmd_adr, FL_WRITING);
 	if (ret) {
@@ -1759,29 +1768,30 @@ static int __xipram do_write_buffer(struct map_info *map, struct flchip *chip,
 	   So we must check here and reset those bits if they're set. Otherwise
 	   we're just pissing in the wind */
 	if (chip->state != FL_STATUS) {
-		map_write(map, CMD(0x70), cmd_adr);
+		cfi_write_cmd(map, 0x70, cmd_adr);
 		chip->state = FL_STATUS;
 	}
 	status = map_read(map, cmd_adr);
 	if (map_word_bitsset(map, status, CMD(0x30))) {
 		xip_enable(map, chip, cmd_adr);
 		printk(KERN_WARNING "SR.4 or SR.5 bits set in buffer write (status %lx). Clearing.\n", status.x[0]);
 		xip_disable(map, chip, cmd_adr);
-		map_write(map, CMD(0x50), cmd_adr);
-		map_write(map, CMD(0x70), cmd_adr);
+		cfi_write_cmd(map, 0x50, cmd_adr);
+		cfi_write_cmd(map, 0x70, cmd_adr);
 	}
 
 	chip->state = FL_WRITING_TO_BUFFER;
-	map_write(map, write_cmd, cmd_adr);
+	cfi_write_cmd(map, (cfi->cfiq->P_ID != P_ID_INTEL_PERFORMANCE) ? 0xe8 : 0xe9,
+		      cmd_adr);
 	ret = WAIT_TIMEOUT(map, chip, cmd_adr, 0, 0);
 	if (ret) {
 		/* Argh. Not ready for write to buffer */
 		map_word Xstatus = map_read(map, cmd_adr);
-		map_write(map, CMD(0x70), cmd_adr);
+		cfi_write_cmd(map, 0x70, cmd_adr);
 		chip->state = FL_STATUS;
 		status = map_read(map, cmd_adr);
-		map_write(map, CMD(0x50), cmd_adr);
-		map_write(map, CMD(0x70), cmd_adr);
+		cfi_write_cmd(map, 0x50, cmd_adr);
+		cfi_write_cmd(map, 0x70, cmd_adr);
 		xip_enable(map, chip, cmd_adr);
 		printk(KERN_ERR "%s: Chip not ready for buffer write. Xstatus = %lx, status = %lx\n",
 				map->name, Xstatus.x[0], status.x[0]);
@@ -1800,7 +1810,7 @@ static int __xipram do_write_buffer(struct map_info *map, struct flchip *chip,
 	}
 
 	/* Write length of data to come */
-	map_write(map, CMD(words), cmd_adr );
+	cfi_write_cmd(map, words, cmd_adr);
 
 	/* Write data */
 	vec = *pvec;
@@ -1837,15 +1847,15 @@ static int __xipram do_write_buffer(struct map_info *map, struct flchip *chip,
 	*pvec_seek = vec_seek;
 
 	/* GO GO GO */
-	map_write(map, CMD(0xd0), cmd_adr);
+	cfi_write_cmd(map, 0xd0, cmd_adr);
 	chip->state = FL_WRITING;
 
 	ret = INVAL_CACHE_AND_WAIT(map, chip, cmd_adr,
 				   initial_adr, initial_len,
 				   chip->buffer_write_time,
 				   chip->buffer_write_time_max);
 	if (ret) {
-		map_write(map, CMD(0x70), cmd_adr);
+		cfi_write_cmd(map, 0x70, cmd_adr);
 		chip->state = FL_STATUS;
 		xip_enable(map, chip, cmd_adr);
 		printk(KERN_ERR "%s: buffer write error (status timeout)\n", map->name);
@@ -1858,8 +1868,8 @@ static int __xipram do_write_buffer(struct map_info *map, struct flchip *chip,
 		unsigned long chipstatus = MERGESTATUS(status);
 
 		/* reset status */
-		map_write(map, CMD(0x50), cmd_adr);
-		map_write(map, CMD(0x70), cmd_adr);
+		cfi_write_cmd(map, 0x50, cmd_adr);
+		cf
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`add_wait_queue`
- 外部函数：`cfi_interleave_is_1`
- 外部函数：`cfi_udelay`
- 外部函数：`cond_resched`
- 外部函数：`cpu_relax`
- 外部函数：`error`
- 外部函数：`local_irq_disable`
- 外部函数：`local_irq_enable`
- 外部函数：`map_read`
- 外部函数：`map_word_andequal`
- 外部函数：`map_word_bitsset`
- 外部函数：`map_write`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_trylock`
- 外部函数：`mutex_unlock`
- 外部函数：`printk`
- 外部函数：`remove_wait_queue`
- 外部函数：`schedule`
- 外部函数：`set_current_state`
- 外部函数：`some`
- 外部函数：`suspended`
- 外部函数：`time_after`
- 外部函数：`wake_up`
- 外部函数：`write`
- 外部函数：`xip_cpu_idle`
- 外部函数：`xip_currtime`
- 外部函数：`xip_disable`
- 外部函数：`xip_elapsed_since`
- 外部函数：`xip_enable`
- 外部函数：`xip_iprefetch`
- 外部函数：`xip_irqpending`
- 大写宏：`CFI_MFR_INTEL`
- 大写宏：`CFI_MFR_SHARP`
- 大写宏：`CMD`
- 大写宏：`CPU`
- 大写宏：`DECLARE_WAITQUEUE`
- 大写宏：`EAGAIN`
- 大写宏：`EIO`
- 大写宏：`EROFS`
- 大写宏：`ETIME`
- 大写宏：`FL_CFI_QUERY`
- 大写宏：`FL_ERASE_SUSPENDING`
- 大写宏：`FL_ERASING`
- 大写宏：`FL_JEDEC_QUERY`
- 大写宏：`FL_OTP_WRITE`
- 大写宏：`FL_POINT`
- 大写宏：`FL_READY`
- 大写宏：`FL_SHUTDOWN`
- 大写宏：`FL_STATUS`
- 大写宏：`FL_SYNCING`
- 大写宏：`FL_WRITING`
- 大写宏：`FL_WRITING_TO_BUFFER`
- 大写宏：`FL_XIP_WHILE_ERASING`
- 大写宏：`FL_XIP_WHILE_WRITING`
- 大写宏：`KERN_ERR`
- 大写宏：`KERN_WARNING`
- 大写宏：`LH28F640BF`
- 大写宏：`MERGESTATUS`
- 大写宏：`NULL`
- 大写宏：`P_ID`
- 大写宏：`P_ID_INTEL_PERFORMANCE`
- 大写宏：`READY`
- 大写宏：`TASK_UNINTERRUPTIBLE`
- 大写宏：`XIP`
- 外部类型：`Argh`
- 外部类型：`As`
- 外部类型：`At`
- 外部类型：`Check`
- 外部类型：`Chip`
- 外部类型：`Chips`
- 外部类型：`Clearing`
- 外部类型：`Disallow`
- 外部类型：`Do`
- 外部类型：`Don`
- 外部类型：`EBs`
- 外部类型：`Erase`
- 外部类型：`Family`
- 外部类型：`FeatureSupport`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Let`
- 外部类型：`Make`
- 外部类型：`Micron`
- 外部类型：`No`
- 外部类型：`Nobody`
- 外部类型：`Not`
- 外部类型：`Note`
- 外部类型：`Numonyx`
- 外部类型：`Only`
- 外部类型：`Otherwise`
- 外部类型：`Prevent`
- 外部类型：`Put`
- 外部类型：`Read`
- 外部类型：`Resume`
- 外部类型：`Sharp`
- 外部类型：`Sleep`
- 外部类型：`So`
- 外部类型：`Someone`
- 外部类型：`Status`
- 外部类型：`Supend`
- 外部类型：`Suspend`
- 外部类型：`SuspendCmdSupport`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Try`
- 外部类型：`Urgh`
- 外部类型：`We`
- 外部类型：`What`
- 外部类型：`Write`
- 外部类型：`Xstatus`
- 外部类型：`Yes`
- 外部类型：`can`
- 外部类型：`cfi_pri_intelext`
- 外部类型：`cfi_private`
- 外部类型：`flchip`
- 外部类型：`flchip_shared`
- 外部类型：`flstate_t`
- 外部类型：`kvec`
- 外部类型：`map_info`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-96e485cf7c` → 本草稿移入 `cases/defect/auto-linux-96e485cf7c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
