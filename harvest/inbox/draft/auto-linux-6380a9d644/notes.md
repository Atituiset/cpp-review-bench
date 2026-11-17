# auto-linux-6380a9d644

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb](https://github.com/torvalds/linux/commit/b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 50 |
| 编译错误数（gcc syntax-only） | 25（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb (https://github.com/torvalds/linux/commit/b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 288；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb Merge tag 'tty-7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/gregkh/tty :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -34,29 +34,6 @@ struct hp300_port {
 static struct hp300_port *hp300_ports;
 #endif
 
-#ifdef CONFIG_HPDCA
-
-static int hpdca_init_one(struct dio_dev *d,
-					const struct dio_device_id *ent);
-static void hpdca_remove_one(struct dio_dev *d);
-
-static struct dio_device_id hpdca_dio_tbl[] = {
-	{ DIO_ID_DCA0 },
-	{ DIO_ID_DCA0REM },
-	{ DIO_ID_DCA1 },
-	{ DIO_ID_DCA1REM },
-	{ 0 }
-};
-
-static struct dio_driver hpdca_driver = {
-	.name      = "hpdca",
-	.id_table  = hpdca_dio_tbl,
-	.probe     = hpdca_init_one,
-	.remove    = hpdca_remove_one,
-};
-
-#endif
-
 static unsigned int num_ports;
 
 extern int hp300_uart_scode;
@@ -197,6 +174,32 @@ static int hpdca_init_one(struct dio_dev *d,
 
 	return 0;
 }
+
+static void hpdca_remove_one(struct dio_dev *d)
+{
+	struct uart_8250_port *uport = dio_get_drvdata(d);
+
+	if (d->resource.start) {
+		/* Disable board-interrupts */
+		out_8(d->resource.start + DIO_VIRADDRBASE + DCA_IC, 0);
+	}
+	serial8250_unregister_port(uport);
+}
+
+static struct dio_device_id hpdca_dio_tbl[] = {
+	{ DIO_ID_DCA0 },
+	{ DIO_ID_DCA0REM },
+	{ DIO_ID_DCA1 },
+	{ DIO_ID_DCA1REM },
+	{ 0 }
+};
+
+static struct dio_driver hpdca_driver = {
+	.name      = "hpdca",
+	.id_table  = hpdca_dio_tbl,
+	.probe     = hpdca_init_one,
+	.remove    = hpdca_remove_one,
+};
 #endif
 
 static int __init hp300_8250_init(void)
@@ -280,19 +283,6 @@ static int __init hp300_8250_init(void)
 	return 0;
 }
 
-#ifdef CONFIG_HPDCA
-static void hpdca_remove_one(struct dio_dev *d)
-{
-	struct uart_8250_port *uport = dio_get_drvdata(d);
-
-	if (d->resource.start) {
-		/* Disable board-interrupts */
-		out_8(d->resource.start + DIO_VIRADDRBASE + DCA_IC, 0);
-	}
-	serial8250_unregister_port(uport);
-}
-#endif
-
 static void __exit hp300_8250_exit(void)
 {
 #ifdef CONFIG_HPAPCI
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_notice`
- 外部函数：`dio_get_drvdata`
- 外部函数：`dio_register_driver`
- 外部函数：`dio_set_drvdata`
- 外部函数：`hpdca_init_one`
- 外部函数：`kfree`
- 外部函数：`kmalloc_obj`
- 外部函数：`memset`
- 外部函数：`out_8`
- 外部函数：`register_serial`
- 外部函数：`serial8250_register_8250_port`
- 外部函数：`serial8250_unregister_port`
- 外部函数：`udelay`
- 大写宏：`APCI`
- 大写宏：`CONFIG_HPAPCI`
- 大写宏：`CONFIG_HPDCA`
- 大写宏：`CONFIG_SERIAL_8250_CONSOLE`
- 大写宏：`DCA`
- 大写宏：`DIO_ID_DCA0`
- 大写宏：`DIO_ID_DCA0REM`
- 大写宏：`DIO_ID_DCA1`
- 大写宏：`DIO_ID_DCA1REM`
- 大写宏：`DIO_VIRADDRBASE`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`HP_400`
- 大写宏：`IS_ERR`
- 大写宏：`MACH_IS_HP300`
- 大写宏：`UPF_BOOT_AUTOCONF`
- 大写宏：`UPF_SHARE_IRQ`
- 大写宏：`UPF_SKIP_TEST`
- 大写宏：`UPIO_MEM`
- 大写宏：`XXX`
- 外部类型：`Already`
- 外部类型：`Any`
- 外部类型：`Apollo`
- 外部类型：`Create`
- 外部类型：`Disable`
- 外部类型：`Domain`
- 外部类型：`Enable`
- 外部类型：`Frodo`
- 外部类型：`Interrupt`
- 外部类型：`Master`
- 外部类型：`Memory`
- 外部类型：`Port`
- 外部类型：`Reset`
- 外部类型：`These`
- 外部类型：`dio_dev`
- 外部类型：`dio_device_id`
- 外部类型：`dio_driver`
- 外部类型：`uart_8250_port`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-6380a9d644` → 本草稿移入 `cases/defect/auto-linux-6380a9d644/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
