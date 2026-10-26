# auto-linux-432aeafbe9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#2270aa5ed5f7981082e4fae4103b8930e0ba106f](https://github.com/torvalds/linux/commit/2270aa5ed5f7981082e4fae4103b8930e0ba106f) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2270aa5ed5f7981082e4fae4103b8930e0ba106f (https://github.com/torvalds/linux/commit/2270aa5ed5f7981082e4fae4103b8930e0ba106f)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 804；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2270aa5ed5f7981082e4fae4103b8930e0ba106f Merge tag 'mips_7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/mips/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -30,6 +30,7 @@
 #include <linux/libfdt.h>
 #include <linux/kexec.h>
 
+#include <asm/machine.h>
 #include <asm/processor.h>
 #include <asm/reboot.h>
 #include <asm/smp-ops.h>
@@ -653,6 +654,24 @@ void octeon_user_io_init(void)
 	write_c0_derraddr1(0);
 }
 
+#ifdef CONFIG_CAVIUM_OCTEON_LOCK_L2
+static bool __init octeon_l2_is_crippled(void)
+{
+	/*
+	 * L2D_FUS3 exists on CN3XXX/CN5XXX only. On OCTEON II the
+	 * crippled-L2 fuses live in MIO_FUS_DAT3[l2c_crip].
+	 */
+	if (OCTEON_IS_MODEL(OCTEON_CN6XXX)) {
+		union cvmx_mio_fus_dat3 fus_dat3;
+
+		fus_dat3.u64 = cvmx_read_csr(CVMX_MIO_FUS_DAT3);
+		return fus_dat3.s.l2c_crip != 0;
+	}
+
+	return cvmx_read_csr(CVMX_L2D_FUS3) & (3ull << 34);
+}
+#endif
+
 /**
  * prom_init - Early entry point for arch setup
  */
@@ -801,7 +820,7 @@ void __init prom_init(void)
 	}
 
 #ifdef CONFIG_CAVIUM_OCTEON_LOCK_L2
-	if (cvmx_read_csr(CVMX_L2D_FUS3) & (3ull << 34)) {
+	if (octeon_l2_is_crippled()) {
 		pr_info("Skipping L2 locking due to reduced L2 cache size\n");
 	} else {
 		uint32_t __maybe_unused ebase = read_c0_ebase() & 0x3ffff000;
@@ -1109,7 +1128,7 @@ EXPORT_SYMBOL(prom_putchar);
 
 void __init prom_free_prom_memory(void)
 {
-	if (OCTEON_IS_MODEL(OCTEON_CN6XXX)) {
+	if (OCTEON_IS_MODEL(OCTEON_CN63XX_PASS1_X)) {
 		/* Check for presence of Core-14449 fix.  */
 		u32 insn;
 		u32 *foo;
@@ -1141,7 +1160,7 @@ void __init octeon_fill_mac_addresses(void);
 
 void __init device_tree_init(void)
 {
-	const void *fdt;
+	const void *fdt = NULL;
 	bool do_prune;
 	bool fill_mac;
 
@@ -1160,12 +1179,28 @@ void __init device_tree_init(void)
 		do_prune = false;
 		fill_mac = false;
 		pr_info("Using passed Device Tree.\n");
-	} else if (OCTEON_IS_MODEL(OCTEON_CN68XX)) {
-		fdt = &__dtb_octeon_68xx_begin;
-		do_prune = true;
-		fill_mac = true;
 	} else {
-		fdt = &__dtb_octeon_3xxx_begin;
+		const struct mips_machine *check_mach;
+
+		for_each_mips_machine(check_mach) {
+			if (!check_mach->detect)
+				continue;
+			if (!check_mach->detect())
+				continue;
+
+			fdt = check_mach->fdt;
+			do_prune = false;
+			fill_mac = true;
+			pr_info("Using compiled-in Device Tree.\n");
+			break;
+		}
+	}
+
+	if (!fdt) {
+		if (OCTEON_IS_MODEL(OCTEON_CN68XX))
+			fdt = &__dtb_octeon_68xx_begin;
+		else
+			fdt = &__dtb_octeon_3xxx_begin;
 		do_prune = true;
 		fill_mac = true;
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cvmx_read_csr`
- 外部函数：`pr_info`
- 外部函数：`read_c0_ebase`
- 外部函数：`write_c0_derraddr1`
- 大写宏：`CONFIG_CAVIUM_OCTEON_LOCK_L2`
- 大写宏：`CVMX_L2D_FUS3`
- 大写宏：`OCTEON_CN68XX`
- 大写宏：`OCTEON_CN6XXX`
- 大写宏：`OCTEON_IS_MODEL`
- 外部类型：`Check`
- 外部类型：`Core`
- 外部类型：`Device`
- 外部类型：`Early`
- 外部类型：`Skipping`
- 外部类型：`Tree`
- 外部类型：`Using`
- 外部类型：`uint32_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-432aeafbe9` → 本草稿移入 `cases/defect/auto-linux-432aeafbe9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
