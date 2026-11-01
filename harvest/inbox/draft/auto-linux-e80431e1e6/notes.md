# auto-linux-e80431e1e6

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#cbbddb6b08cc9900a87cccaa9b22bbe06aa4f50a](https://github.com/torvalds/linux/commit/cbbddb6b08cc9900a87cccaa9b22bbe06aa4f50a) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 444 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cbbddb6b08cc9900a87cccaa9b22bbe06aa4f50a (https://github.com/torvalds/linux/commit/cbbddb6b08cc9900a87cccaa9b22bbe06aa4f50a)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 1369；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cbbddb6b08cc9900a87cccaa9b22bbe06aa4f50a Merge tag 'power-utilities-2026.10.31' of git://git.kernel.org/pub/scm/linux/kernel/git/lenb/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -98,6 +98,8 @@
 
 #define ROUND_UP_TO_PAGE_SIZE(n) (((n) + 0x1000UL-1UL) & ~(0x1000UL-1UL))
 
+FILE *outf;
+
 enum counter_scope { SCOPE_CPU, SCOPE_CORE, SCOPE_PACKAGE };
 enum counter_type { COUNTER_ITEMS, COUNTER_CYCLES, COUNTER_SECONDS, COUNTER_USEC, COUNTER_K2M };
 enum counter_format { FORMAT_RAW, FORMAT_DELTA, FORMAT_PERCENT, FORMAT_AVERAGE };
@@ -138,8 +140,6 @@ struct msr_counter {
 	enum counter_format format;
 	struct msr_counter *next;
 	unsigned int flags;
-#define	FLAGS_HIDE	(1 << 0)
-#define	FLAGS_SHOW	(1 << 1)
 #define	SYSFS_PERCPU	(1 << 1)
 };
 static int use_android_msr_path;
@@ -292,33 +292,53 @@ enum bic_names {
 	MAX_BIC
 };
 
-void print_bic_set(char *s, cpu_set_t *set)
+typedef cpu_set_t bic_set_t;	/* implement bic_set_t using cpu_set_t */
+
+/*
+ * debug helper
+ */
+static void print_bic_set(const char *s, const bic_set_t *set)
 {
 	int i;
 
 	assert(MAX_BIC < CPU_SETSIZE);
 
-	printf("%s:", s);
+	fprintf(outf, "%s:", s);
 
 	for (i = 0; i < MAX_BIC; ++i) {
 
 		if (CPU_ISSET(i, set))
-			printf(" %s", bic[i].name);
+			fprintf(outf, " %s", bic[i].name);
 	}
-	putchar('\n');
+	fputc('\n', outf);
 }
 
-static cpu_set_t bic_group_topology;
-static cpu_set_t bic_group_thermal_pwr;
-static cpu_set_t bic_group_frequency;
-static cpu_set_t bic_group_hw_idle;
-static cpu_set_t bic_group_sw_idle;
-static cpu_set_t bic_group_idle;
-static cpu_set_t bic_group_cache;
-static cpu_set_t bic_group_other;
-static cpu_set_t bic_group_disabled_by_default;
-static cpu_set_t bic_enabled;
-static cpu_set_t bic_present;
+static bic_set_t bic_group_topology;
+static bic_set_t bic_group_thermal_pwr;
+static bic_set_t bic_group_frequency;
+static bic_set_t bic_group_hw_idle;
+static bic_set_t bic_group_sw_idle;
+static bic_set_t bic_group_idle;
+static bic_set_t bic_group_cache;
+static bic_set_t bic_group_other;
+static bic_set_t bic_group_disabled_by_default;
+static bic_set_t bic_enabled;
+static bic_set_t bic_present;
+
+static void dump_bic_sets(void)
+{
+	print_bic_set("bic_enabled", &bic_enabled);
+	print_bic_set("bic_present", &bic_present);
+	print_bic_set("bic_group_topology", &bic_group_topology);
+	print_bic_set("bic_group_thermal_pwr", &bic_group_thermal_pwr);
+	print_bic_set("bic_group_frequency", &bic_group_frequency);
+	print_bic_set("bic_group_hw_idle", &bic_group_hw_idle);
+	print_bic_set("bic_group_sw_idle", &bic_group_sw_idle);
+	print_bic_set("bic_group_idle", &bic_group_idle);
+	print_bic_set("bic_group_cache", &bic_group_cache);
+	print_bic_set("bic_group_other", &bic_group_other);
+	print_bic_set("bic_group_disabled_by_default", &bic_group_disabled_by_default);
+}
 
 /* modify */
 #define BIC_INIT(set) CPU_ZERO(set)
@@ -334,7 +354,7 @@ static cpu_set_t bic_present;
 #define DO_BIC_READ(COUNTER_NUMBER) CPU_ISSET(COUNTER_NUMBER, &bic_present)
 #define DO_BIC(COUNTER_NUMBER) (CPU_ISSET(COUNTER_NUMBER, &bic_enabled) && CPU_ISSET(COUNTER_NUMBER, &bic_present))
 
-static void bic_set_all(cpu_set_t *set)
+static void bic_set_all(bic_set_t *set)
 {
 	int i;
 
@@ -348,7 +368,7 @@ static void bic_set_all(cpu_set_t *set)
  * bic_clear_bits()
  * clear all the bits from "clr" in "dst"
  */
-static void bic_clear_bits(cpu_set_t *dst, cpu_set_t *clr)
+static void bic_clear_bits(bic_set_t *dst, bic_set_t *clr)
 {
 	int i;
 
@@ -379,6 +399,11 @@ static void bic_groups_init(void)
 	SET_BIC(BIC_CorWatt, &bic_group_thermal_pwr);
 	SET_BIC(BIC_GFXWatt, &bic_group_thermal_pwr);
 	SET_BIC(BIC_RAMWatt, &bic_group_thermal_pwr);
+	SET_BIC(BIC_Pkg_J, &bic_group_thermal_pwr);
+	SET_BIC(BIC_Cor_J, &bic_group_thermal_pwr);
+	SET_BIC(BIC_GFX_J, &bic_group_thermal_pwr);
+	SET_BIC(BIC_RAM_J, &bic_group_thermal_pwr);
+	SET_BIC(BIC_Sys_J, &bic_group_thermal_pwr);
 	SET_BIC(BIC_PKG__, &bic_group_thermal_pwr);
 	SET_BIC(BIC_RAM__, &bic_group_thermal_pwr);
 	SET_BIC(BIC_SysWatt, &bic_group_thermal_pwr);
@@ -485,7 +510,6 @@ static void bic_groups_init(void)
 #define PCLUNL 15		/* Unlimited */
 
 char *proc_stat = "/proc/stat";
-FILE *out
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__builtin_unreachable`
- 外部函数：`__cpuid`
- 外部函数：`__cpuid_count`
- 外部函数：`__must_be_array`
- 外部函数：`abs`
- 外部函数：`access`
- 外部函数：`assert`
- 外部函数：`atoi`
- 外部函数：`calloc`
- 外部函数：`close`
- 外部函数：`closedir`
- 外部函数：`column`
- 外部函数：`dirfd`
- 外部函数：`err`
- 外部函数：`errx`
- 外部函数：`exit`
- 外部函数：`fclose`
- 外部函数：`fdopen`
- 外部函数：`fflush`
- 外部函数：`fopen`
- 外部函数：`fprintf`
- 外部函数：`fputs`
- 外部函数：`free`
- 外部函数：`fscanf`
- 外部函数：`fstat`
- 外部函数：`func`
- 外部函数：`getc`
- 外部函数：`getline`
- 外部函数：`getopt_long_only`
- 外部函数：`gettimeofday`
- 外部函数：`int`
- 外部函数：`ldexp`
- 外部函数：`make_perf_counter_info`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`mmap`
- 外部函数：`msr`
- 外部函数：`nan`
- 外部函数：`open`
- 外部函数：`openat`
- 外部函数：`opendir`
- 外部函数：`perf_data`
- 外部函数：`perror`
- 外部函数：`pread`
- 外部函数：`printf`
- 外部函数：`putchar`
- 外部函数：`read`
- 外部函数：`rewind`
- 外部函数：`s`
- 外部函数：`sabled`
- 外部函数：`scandir`
- 外部函数：`sched_getcpu`
- 外部函数：`sched_setaffinity`
- 外部函数：`snprintf`
- 外部函数：`sprintf`
- 外部函数：`sscanf`
- 外部函数：`strchr`
- 外部函数：`strcmp`
- 外部函数：`strerror`
- 外部函数：`strlen`
- 外部函数：`strncmp`
- 外部函数：`strncpy`
- 外部函数：`strtod`
- 外部函数：`strtoul`
- 外部函数：`syscall`
- 外部函数：`timer_create`
- 外部函数：`timer_delete`
- 外部函数：`timer_settime`
- 外部函数：`u`
- 外部函数：`usec`
- 外部函数：`volatile`
- 外部函数：`warn`
- 外部函数：`warnx`
- 外部函数：`write`
- 大写宏：`ALL`
- 大写宏：`API`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`BCLK`
- 大写宏：`BIC`
- 大写宏：`BIC_APIC`
- 大写宏：`BIC_CORE_THROT_CNT`
- 大写宏：`BIC_CPUGFX`
- 大写宏：`BIC_CPU_LPI`
- 大写宏：`BIC_IPC`
- 大写宏：`BIC_IRQ`
- 大写宏：`BIC_L2_HIT`
- 大写宏：`BIC_L2_MRPS`
- 大写宏：`BIC_LLC_HIT`
- 大写宏：`BIC_LLC_MRPS`
- 大写宏：`BIC_NMI`
- 大写宏：`BIC_PKG__`
- 大写宏：`BIC_RAM__`
- 大写宏：`BIC_SMI`
- 大写宏：`BIC_SYS_LPI`
- 大写宏：`BIC_UNCORE_MHZ`
- 大写宏：`BIC_USEC`
- 大写宏：`BIC_X2APIC`
- 大写宏：`BIOS`
- 大写宏：`BUG`
- 大写宏：`BUILD_BUG_ON`
- 大写宏：`CC6`
- 大写宏：`CC7`
- 大写宏：`CCSTATE_RCI_INDEX_C1_RESIDENCY`
- 大写宏：`CCSTATE_RCI_INDEX_C3_RESIDENCY`
- 大写宏：`CCSTATE_RCI_INDEX_C6_RESIDENCY`
- 大写宏：`CCSTATE_RCI_INDEX_C7_RESIDENCY`
- 大写宏：`CLOCK_REALTIME`
- 大写宏：`COMMAND`
- 大写宏：`COR`
- 大写宏：`COUNTER_CYCLES`
- 大写宏：`COUNTER_ITEMS`
- 大写宏：`COUNTER_K2M`
- 大写宏：`COUNTER_NUMBER`
- 大写宏：`COUNTER_SECONDS`
- 大写宏：`COUNTER_SOURCE_MSR`
- 大写宏：`COUNTER_SOURCE_NONE`
- 大写宏：`COUNTER_SOURCE_PERF`
- 大写宏：`COUNTER_USEC`
- 大写宏：`CPU`
- 大写宏：`CPUID`
- 大写宏：`CPU_ALLOC`
- 大写宏：`CPU_ALLOC_SIZE`
- 大写宏：`CPU_CLR`
- 大写宏：`CPU_COUNT_S`
- 大写宏：`CPU_FREE`
- 大写宏：`CPU_ISSET`
- 大写宏：`CPU_ISSET_S`
- 大写宏：`CPU_SET`
- 大写宏：`CPU_SETSIZE`
- 大写宏：`CPU_SET_S`
- 大写宏：`CPU_ZERO`
- 大写宏：`CPU_ZERO_S`
- 大写宏：`CWF`
- 大写宏：`DIR`
- 大写宏：`DIS`
- 大写宏：`DRAM`
- 大写宏：`DTS`
- 大写宏：`EINVAL`
- 大写宏：`ENODEV`
- 大写宏：`EPB`
- 大写宏：`FEAT_CTL_LOCKED`
- 大写宏：`FILE`
- 大写宏：`FIXME`
- 大写宏：`FORMAT_AVERAGE`
- 大写宏：`FORMAT_DELTA`
- 大写宏：`FORMAT_PERCENT`
- 大写宏：`FORMAT_RAW`
- 大写宏：`GENMASK`
- 大写宏：`GFX`
- 大写宏：`GFX_MAX`
- 大写宏：`GRR`
- 大写宏：`GUID`
- 大写宏：`HIDE_LIST`
- 大写宏：`HIT`
- 大写宏：`HWP`
- 大写宏：`HWP_GUARANTEED_PERF`
- 大写宏：`HWP_HIGHEST_PERF`
- 大写宏：`HWP_LOWEST_PERF`
- 大写宏：`HWP_MOSTEFFICIENT_PERF`
- 大写宏：`IDX_DRAM_ENERGY`
- 大写宏：`IDX_DRAM_PERF`
- 大写宏：`IDX_PKG_ENERGY`
- 大写宏：`IDX_PKG_PERF`
- 大写宏：`IDX_PP0_ENERGY`
- 大写宏：`IDX_PP1_ENERGY`
- 大写宏：`IDX_PSYS_ENERGY`
- 大写宏：`INTEL_ALDERLAKE`
- 大写宏：`INTEL_ALDERLAKE_L`
- 大写宏：`INTEL_ARROWLAKE`
- 大写宏：`INTEL_ATOM_CRESTMONT`
- 大写宏：`INTEL_ATOM_DARKMONT_X`
- 大写宏：`INTEL_LUNARLAKE_M`
- 大写宏：`INTEL_NOVALAKE`
- 大写宏：`INTEL_NOVALAKE_L`
- 大写宏：`INTEL_PANTHERLAKE_L`
- 大写宏：`INTEL_RAPTORLAKE`
- 大写宏：`INTEL_WILDCATLAKE_L`
- 大写宏：`IPC`
- 大写宏：`IRQ`
- 大写宏：`L2_REQUEST`
- 大写宏：`LLC`
- 大写宏：`M0X`
- 大写宏：`MAP_FAILED`
- 大写宏：`MAP_SHARED`
- 大写宏：`MAX_BIC`
- 大写宏：`MC6`
- 大写宏：`MMIO`
- 大写宏：`MSR`
- 大写宏：`MSR_CC6_DEMOTION_POLICY_CONFIG`
- 大写宏：`MSR_CORE_C6_RESIDENCY`
- 大写宏：`MSR_DRAM_ENERGY_STATUS`
- 大写宏：`MSR_DRAM_PERF_STATUS`
- 大写宏：`MSR_DRAM_POWER_INFO`
- 大写宏：`MSR_DRAM_POWER_LIMIT`
- 大写宏：`MSR_FSB_FREQ`
- 大写宏：`MSR_HWP_CAPABILITIES`
- 大写宏：`MSR_HWP_INTERRUPT`
- 大写宏：`MSR_HWP_REQUEST`
- 大写宏：`MSR_HWP_REQUEST_PKG`
- 大写宏：`MSR_HWP_STATUS`
- 大写宏：`MSR_IA32_FEATURE_CONTROL`
- 大写宏：`MSR_IA32_FEAT_CTL`
- 大写宏：`MSR_IA32_MISC_ENABLE`
- 大写宏：`MSR_IA32_MISC_ENABLE_ENHANCED_SPEEDSTEP`
- 大写宏：`MSR_IA32_MISC_ENABLE_MWAIT`
- 大写宏：`MSR_IA32_MISC_ENABLE_PREFETCH_DISABLE`
- 大写宏：`MSR_IA32_MISC_ENABLE_TM1`
- 大写宏：`MSR_IA32_MISC_ENABLE_TURBO_DISABLE`
- 大写宏：`MSR_IA32_PACKAGE_THERM_INTERRUPT`
- 大写宏：`MSR_IA32_PACKAGE_THERM_STATUS`
- 大写宏：`MSR_IA32_TEMPERATURE_TARGET`
- 大写宏：`MSR_IA32_THERM_INTERRUPT`
- 大写宏：`MSR_IA32_THERM_STATUS`
- 大写宏：`MSR_IA32_UCODE_REV`
- 大写宏：`MSR_MC6_DEMOTION_POLICY_CONFIG`
- 大写宏：`MSR_MISC_FEATURE_CONTROL`
- 大写宏：`MSR_MISC_PWR_MGMT`
- 大写宏：`MSR_MODULE_C6_RES_MS`
- 大写宏：`MSR_PKG_ANY_CORE_C0_RES`
- 大写宏：`MSR_PKG_ANY_GFXE_C0_RES`
- 大写宏：`MSR_PKG_BOTH_CORE_GFXE_C0_RES`
- 大写宏：`MSR_PKG_ENERGY_STATUS`
- 大写宏：`MSR_PKG_PERF_STATUS`
- 大写宏：`MSR_PKG_POWER_INFO`
- 大写宏：`MSR_PKG_POWER_LIMIT`
- 大写宏：`MSR_PKG_WEIGHTED_CORE_C0_RES`
- 大写宏：`MSR_PLATFORM_ENERGY_STATUS`
- 大写宏：`MSR_PLATFORM_INFO`
- 大写宏：`MSR_PM_ENABLE`
- 大写宏：`MSR_PP0_ENERGY_STATUS`
- 大写宏：`MSR_PP0_POLICY`
- 大写宏：`MSR_PP0_POWER_LIMIT`
- 大写宏：`MSR_PP1_ENERGY_STATUS`
- 大写宏：`MSR_PP1_POLICY`
- 大写宏：`MSR_PP1_POWER_LIMIT`
- 大写宏：`MSR_RAPL_POWER_UNIT`
- 大写宏：`MSR_RCI_INDEX_APERF`
- 大写宏：`MSR_RCI_INDEX_MPERF`
- 大写宏：`MSR_RCI_INDEX_SMI`
- 大写宏：`MSR_RING_PERF_LIMIT_REASONS`
- 大写宏：`MSR_VR_CURRENT_CONFIG`
- 大写宏：`MY_TSC`
- 大写宏：`NHM_C1_AUTO_DEMOTE`
- 大写宏：`NHM_C3_AUTO_DEMOTE`
- 大写宏：`NMI`
- 大写宏：`NSFOD`
- 大写宏：`NULL`
- 大写宏：`NUM_CSTATE_COUNTERS`
- 大写宏：`NUM_MSR_COUNTERS`
- 大写宏：`NUM_RAPL_COUNTERS`
- 大写宏：`OOB`
- 大写宏：`OPTIONS`
- 大写宏：`O_DIRECTORY`
- 大写宏：`O_RDONLY`
- 大写宏：`PATH_MAX`
- 大写宏：`PCSTATE_RCI_INDEX_C10_RESIDENCY`
- 大写宏：`PCSTATE_RCI_INDEX_C2_RESIDENCY`
- 大写宏：`PCSTATE_RCI_INDEX_C3_RESIDENCY`
- 大写宏：`PCSTATE_RCI_INDEX_C6_RESIDENCY`
- 大写宏：`PCSTATE_RCI_INDEX_C7_RESIDENCY`
- 大写宏：`PCSTATE_RCI_INDEX_C8_RESIDENCY`
- 大写宏：`PCSTATE_RCI_INDEX_C9_RESIDENCY`
- 大写宏：`PERF_COUNT_HW_INSTRUCTIONS`
- 大写宏：`PERF_FORMAT_GROUP`
- 大写宏：`PERF_SAMPLE_IDENTIFIER`
- 大写宏：`PERF_TYPE_HARDWARE`
- 大写宏：`PKG`
- 大写宏：`PLR_GFX`
- 大写宏：`PLR_RING`
- 大写宏：`PMT`
- 大写宏：`PMT_OPEN_REQUIRED`
- 大写宏：`PMT_OPEN_TRY`
- 大写宏：`PMT_TYPE_RAW`
- 大写宏：`PMT_TYPE_TCORE_CLOCK`
- 大写宏：`PMT_TYPE_XTAL_TIME`
- 大写宏：`PROCHOT`
- 大写宏：`PROT_READ`
- 大写宏：`PSYS`
- 大写宏：`PTM`
- 大写宏：`RAM`
- 大写宏：`RAPL`
- 大写宏：`RAPL_AMD_CORE_ENERGY_STAT`
- 大写宏：`RAPL_AMD_PKG_ENERGY_STAT`
- 大写宏：`RAPL_AMD_PWR_UNIT`
- 大写宏：`RAPL_CORE_POLICY`
- 大写宏：`RAPL_CORE_POWER_LIMIT`
- 大写宏：`RAPL_DRAM_ENERGY_STATUS`
- 大写宏：`RAPL_DRAM_POWER_INFO`
- 大写宏：`RAPL_DRAM_POWER_LIMIT`
- 大写宏：`RAPL_GFX_ENERGY_STATUS`
- 大写宏：`RAPL_GFX_POWER_LIMIT`
- 大写宏：`RAPL_PKG_ENERGY`
- 大写宏：`RAPL_PKG_ENERGY_STATUS`
- 大写宏：`RAPL_PKG_POWER_INFO`
- 大写宏：`RAPL_PKG_POWER_LIMIT`
- 大写宏：`RAPL_RCI_INDEX_CORE_ENERGY`
- 大写宏：`RAPL_RCI_INDEX_DRAM`
- 大写宏：`RAPL_RCI_INDEX_DRAM_PERF_STATUS`
- 大写宏：`RAPL_RCI_INDEX_ENERGY_CORES`
- 大写宏：`RAPL_RCI_INDEX_ENERGY_PKG`
- 大写宏：`RAPL_RCI_INDEX_ENERGY_PLATFORM`
- 大写宏：`RAPL_RCI_INDEX_GFX`
- 大写宏：`RAPL_RCI_INDEX_PKG_PERF_STATUS`
- 大写宏：`RAPL_UNIT_INVALID`
- 大写宏：`RAPL_UNIT_JOULES`
- 大写宏：`RAPL_UNIT_WATTS`
- 大写宏：`REFS`
- 大写宏：`R_OK`
- 大写宏：`SCOPE_CORE`
- 大写宏：`SCOPE_CPU`
- 大写宏：`SCOPE_PACKAGE`
- 大写宏：`SGX`
- 大写宏：`SHOW_LIST`
- 大写宏：`SIGEV_THREAD`
- 大写宏：`SLM`
- 大写宏：`SMI`
- 大写宏：`SMT`
- 大写宏：`SNB_C1_AUTO_UNDEMOTE`
- 大写宏：`SNB_C3_AUTO_UNDEMOTE`
- 大写宏：`TCC`
- 大写宏：`TDP`
- 大写宏：`TSC`
- 大写宏：`UCODE`
- 大写宏：`XTAL`
- 大写宏：`ZERO`
- 外部类型：`Active`
- 外部类型：`Allocate`
- 外部类型：`Amps`
- 外部类型：`Any`
- 外部类型：`Atom`
- 外部类型：`Auto`
- 外部类型：`BIC_Any_c0`
- 外部类型：`BIC_CPU_c7`
- 外部类型：`BIC_CorWatt`
- 外部类型：`BIC_CoreTmp`
- 外部类型：`BIC_GFXACTMHz`
- 外部类型：`BIC_GFXMHz`
- 外部类型：`BIC_GFXWatt`
- 外部类型：`BIC_GFX_c0`
- 外部类型：`BIC_GFX_rc6`
- 外部类型：`BIC_Mod_c6`
- 外部类型：`BIC_PkgTmp`
- 外部类型：`BIC_Pkgpc3`
- 外部类型：`BIC_Pkgpc6`
- 外部类型：`BIC_Pkgpc7`
- 外部类型：`BIC_RAMWatt`
- 外部类型：`BIC_SAMACTMHz`
- 外部类型：`BIC_SAMMHz`
- 外部类型：`BIC_SAM_mc6`
- 外部类型：`BIC_SysWatt`
- 外部类型：`BIC_Totl_c0`
- 外部类型：`Base`
- 外部类型：`Brown`
- 外部类型：`By`
- 外部类型：`CPUs`
- 外部类型：`Can`
- 外部类型：`Celsius`
- 外部类型：`Circuit`
- 外部类型：`Consume`
- 外部类型：`Control`
- 外部类型：`Copy`
- 外部类型：`Cores`
- 外部类型：`Could`
- 外部类型：`Dde`
- 外部类型：`Demotion`
- 外部类型：`Dis`
- 外部类型：`Disabling`
- 外部类型：`Dump`
- 外部类型：`EIST_Coordination`
- 外部类型：`Each`
- 外部类型：`Expected`
- 外部类型：`Failed`
- 外部类型：`For`
- 外部类型：`Force`
- 外部类型：`Forced`
- 外部类型：`Format`
- 外部类型：`Frequency`
- 外部类型：`GFXPwr`
- 外部类型：`GFX_ACTMHz`
- 外部类型：`GFX_MHz`
- 外部类型：`GFX_rc6`
- 外部类型：`Give`
- 外部类型：`Graphics`
- 外部类型：`Helper`
- 外部类型：`Hit`
- 外部类型：`How`
- 外部类型：`Hypervisor`
- 外部类型：`Idle`
- 外部类型：`If`
- 外部类型：`Initialize`
- 外部类型：`Insanely`
- 外部类型：`Invalid`
- 外部类型：`Jn`
- 外部类型：`Joules`
- 外部类型：`Kernel`
- 外部类型：`Len`
- 外部类型：`Limit`
- 外部类型：`Logged`
- 外部类型：`Low`
- 外部类型：`M0x`
- 外部类型：`MHz`
- 外部类型：`MSRs`
- 外部类型：`Make`
- 外部类型：`Mcycles`
- 外部类型：`Mhz`
- 外部类型：`Minus`
- 外部类型：`Model`
- 外部类型：`No`
- 外部类型：`Offset`
- 外部类型：`Open`
- 外部类型：`Otherwise`
- 外部类型：`Overcommit`
- 外部类型：`Parse`
- 外部类型：`Parsed`
- 外部类型：`PkgPwrL1`
- 外部类型：`PkgPwrL2`
- 外部类型：`Pointer`
- 外部类型：`Power`
- 外部类型：`Prefer`
- 外部类型：`Prefetch`
- 外部类型：`Read`
- 外部类型：`Reading`
- 外部类型：`Requested`
- 外部类型：`SAM_ACTMHz`
- 外部类型：`SAM_MHz`
- 外部类型：`SAM_mc6`
- 外部类型：`Seq`
- 外部类型：`Set`
- 外部类型：`Signal`
- 外部类型：`Skip`
- 外部类型：`Some`
- 外部类型：`Start`
- 外部类型：`Summary`
- 外部类型：`System`
- 外部类型：`Test`
- 外部类型：`The`
- 外部类型：`Therm`
- 外部类型：`ThermStatus`
- 外部类型：`Thermal`
- 外部类型：`This`
- 外部类型：`Throttle`
- 外部类型：`Time_Of_Day_Seconds`
- 外部类型：`Timer`
- 外部类型：`Turbostat`
- 外部类型：`Type`
- 外部类型：`UNdemote`
- 外部类型：`Uncore`
- 外部类型：`Unlimited`
- 外部类型：`Usage`
- 外部类型：`User`
- 外部类型：`Validate`
- 外部类型：`Watts`
- 外部类型：`Weighted`
- 外部类型：`What`
- 外部类型：`You`
- 外部类型：`core_data`
- 外部类型：`cpu_set_t`
- 外部类型：`cstate_counter_info_t`
- 外部类型：`dirent`
- 外部类型：`itimerspec`
- 外部类型：`msr_counter_info_t`
- 外部类型：`msr_sum_array`
- 外部类型：`off_t`
- 外部类型：`option`
- 外部类型：`perf_event_attr`
- 外部类型：`pid_t`
- 外部类型：`pmt_diriter_t`
- 外部类型：`rapl_counter_info_t`
- 外部类型：`sigevent`
- 外部类型：`sigval`
- 外部类型：`size_t`
- 外部类型：`ssize_t`
- 外部类型：`stat`
- 外部类型：`sysfs_path`
- 外部类型：`timeval`
- 外部类型：`timezone`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e80431e1e6` → 本草稿移入 `cases/defect/auto-linux-e80431e1e6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
