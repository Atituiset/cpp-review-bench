# auto-linux-ecf2763dbe

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
| 外部依赖数（dep_count） | 56 |
| 编译错误数（gcc syntax-only） | 66（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cbbddb6b08cc9900a87cccaa9b22bbe06aa4f50a (https://github.com/torvalds/linux/commit/cbbddb6b08cc9900a87cccaa9b22bbe06aa4f50a)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 473；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cbbddb6b08cc9900a87cccaa9b22bbe06aa4f50a Merge tag 'power-utilities-2026.10.31' of git://git.kernel.org/pub/scm/linux/kernel/git/lenb/linux :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -9,17 +9,22 @@
  */
 
 #define _GNU_SOURCE
-#include MSRHEADER
+/*
+ * MSRHEADER macro should be defined by the build system to include the
+ * appropriate MSR definitions header file. Typical usage:
+ * gcc -DMSRHEADER=\"msr-index.h\" or similar platform-specific MSR header
+ */
+#include MSRHEADER  /* e.g., msr-index.h with MSR_* constant definitions */
 #include <stdio.h>
 #include <unistd.h>
 #include <sys/types.h>
 #include <sched.h>
 #include <sys/stat.h>
+#include <sys/wait.h>
 #include <sys/resource.h>
 #include <getopt.h>
 #include <err.h>
 #include <fcntl.h>
-#include <signal.h>
 #include <sys/time.h>
 #include <limits.h>
 #include <stdlib.h>
@@ -33,6 +38,13 @@
 #define	OPTARG_BALANCE_PERFORMANCE	(INT_MAX - 4)
 #define	OPTARG_PERFORMANCE		(INT_MAX - 5)
 
+/* Maximum HWP activity window duration in microseconds */
+#define HWP_WINDOW_MAX_USEC 1270000000U
+
+/* Nominal bus clock frequency in MHz - assumed for all HWP systems */
+#define BCLK_MHZ 100
+#define KHZ_PER_MHZ 1000
+
 struct msr_hwp_cap {
 	unsigned char highest;
 	unsigned char guaranteed;
@@ -98,6 +110,7 @@ unsigned char update_platform_profile;
 int soc_slider_balance;
 int soc_slider_offset;
 char platform_profile[64];
+char msr_path[64];
 
 #define PATH_TO_CPU "/sys/devices/system/cpu/"
 #define SYSFS_PATH_MAX 255
@@ -139,11 +152,13 @@ void usage(void)
 int ratio_2_msr_perf(int ratio)
 {
 	int msr_perf;
+	unsigned long long temp;
 
 	if (!bdx_highest_ratio)
 		return ratio;
 
-	msr_perf = ratio * 255 / bdx_highest_ratio;
+	temp = (unsigned long long)ratio * 255;
+	msr_perf = temp / bdx_highest_ratio;
 
 	if (debug)
 		fprintf(stderr, "%d = ratio_to_msr_perf(%d)\n", msr_perf, ratio);
@@ -189,7 +204,7 @@ int parse_cmdline_epb(int i)
 		return ENERGY_PERF_BIAS_PERFORMANCE;
 	}
 	if (i < 0 || i > ENERGY_PERF_BIAS_POWERSAVE)
-		errx(1, "--epb must be from 0 to 15");
+		errx(1, "--epb must be 0 through 15");
 	return i;
 }
 
@@ -255,7 +270,7 @@ int parse_cmdline_hwp_desired(int i)
 	return i;
 }
 
-int parse_cmdline_hwp_window(int i)
+unsigned int parse_cmdline_hwp_window(unsigned int i)
 {
 	unsigned int exponent;
 
@@ -269,21 +284,21 @@ int parse_cmdline_hwp_window(int i)
 	case OPTARG_PERFORMANCE:
 		return 0;
 	}
-	if (i < 0 || i > 1270000000) {
-		fprintf(stderr, "--hwp-window: 0 for auto; 1 - 1270000000 usec for window duration\n");
+	if (i > HWP_WINDOW_MAX_USEC) {
+		fprintf(stderr, "--hwp-window: 0 for auto; 1 - %u usec for window duration\n", HWP_WINDOW_MAX_USEC);
 		usage();
 	}
 	for (exponent = 0;; ++exponent) {
 		if (debug)
-			printf("%d 10^%d\n", i, exponent);
+			printf("%u 10^%d\n", i, exponent);
 
 		if (i <= 127)
 			break;
 
 		i = i / 10;
 	}
 	if (debug)
-		fprintf(stderr, "%d*10^%d: 0x%x\n", i, exponent, (exponent << 7) | i);
+		fprintf(stderr, "%u*10^%d: 0x%x\n", i, exponent, (exponent << 7) | i);
 
 	return (exponent << 7) | i;
 }
@@ -303,8 +318,8 @@ int parse_cmdline_hwp_epp(int i)
 	case OPTARG_PERFORMANCE:
 		return HWP_EPP_PERFORMANCE;
 	}
-	if (i < 0 || i > 0xff) {
-		fprintf(stderr, "--hwp-epp must be from 0 to 0xff\n");
+	if (i < 0 || i > 255) {
+		fprintf(stderr, "--hwp-epp must be 0 through 255\n");
 		usage();
 	}
 	return i;
@@ -330,42 +345,36 @@ int parse_cmdline_turbo(int i)
 	return i;
 }
 
-int parse_optarg_string(char *s)
+long parse_optarg_string(const char *s)
 {
-	int i;
+	long i;
 	char *endptr;
 
-	if (!strncmp(s, "default", 7))
+	if (strcmp(s, "default") == 0)
 		return OPTARG_NORMAL;
 
-	if (!strncmp(s, "normal", 6))
+	if (strcmp(s, "normal") == 0)
 		return OPTARG_NORMAL;
 
-	if (!strncmp(s, "power", 9))
+	if (strcmp(s, "power") == 0)
 		return OPTARG_POWER;
 
-	if (!strncmp(s, "balance-power", 17))
+	if (strcmp(s, "balance-power") == 0)
 		return OPTARG_BALANCE_POWER;
 
-	if (!strncmp(s, "balance-performance", 19))
+	if (strcmp(s, "balance-performance") == 0)
 		return OPTARG_BALANCE_PERFORMANCE;
 
-	if (!strncmp(s, "performance", 11))
+	if (strcmp(s, "performance") == 0)
 		return OPTARG_PERFORMANCE;
 
+	e
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`d`
- 外部函数：`enable`
- 外部函数：`errx`
- 外部函数：`exit`
- 外部函数：`fprintf`
- 外部函数：`func`
- 外部函数：`msr_perf_ratio`
- 外部函数：`printf`
- 外部函数：`ratio_to_msr_perf`
- 外部函数：`strncmp`
- 外部函数：`strtol`
- 大写宏：`BIAS`
- 大写宏：`CPU_SET_S`
- 大写宏：`ENERGY_PERF_BIAS_PERFORMANCE`
- 大写宏：`ENERGY_PERF_BIAS_POWERSAVE`
- 大写宏：`ERANGE`
- 大写宏：`HWP_EPP_PERFORMANCE`
- 大写宏：`INT_MAX`
- 大写宏：`LONG_MAX`
- 大写宏：`LONG_MIN`
- 大写宏：`MSRHEADER`
- 外部类型：`Energy`
- 外部类型：`Exceeds`
- 外部类型：`Performance`
- 外部类型：`Requested`
- 外部类型：`Specify`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-ecf2763dbe` → 本草稿移入 `cases/defect/auto-linux-ecf2763dbe/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
