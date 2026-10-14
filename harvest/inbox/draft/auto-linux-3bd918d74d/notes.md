# auto-linux-3bd918d74d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#cbc08c20479c6da3db077f9b170bb8cc2d09df20](https://github.com/torvalds/linux/commit/cbc08c20479c6da3db077f9b170bb8cc2d09df20) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 46 |
| 编译错误数（gcc syntax-only） | 48（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cbc08c20479c6da3db077f9b170bb8cc2d09df20 (https://github.com/torvalds/linux/commit/cbc08c20479c6da3db077f9b170bb8cc2d09df20)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 6323；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cbc08c20479c6da3db077f9b170bb8cc2d09df20 Merge tag 'turbostat-2026.09.28' of git://git.kernel.org/pub/scm/linux/kernel/git/lenb/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2034,8 +2034,11 @@ const struct dirent *pmt_diriter_begin(struct pmt_diriter_t *iter, const char *p
 			return NULL;
 
 		num_names = scandir(pmt_root_path, &iter->namelist, pmt_telemdir_filter, pmt_telemdir_sort);
-		if (num_names == -1)
+		if (num_names == -1) {
+			closedir(iter->dir);
+			iter->dir = NULL;
 			return NULL;
+		}
 	}
 
 	iter->current_name_idx = 0;
@@ -2063,8 +2066,10 @@ void pmt_diriter_remove(struct pmt_diriter_t *iter)
 	iter->num_names = 0;
 	iter->current_name_idx = 0;
 
-	closedir(iter->dir);
-	iter->dir = NULL;
+	if (iter->dir) {
+		closedir(iter->dir);
+		iter->dir = NULL;
+	}
 }
 
 unsigned int pmt_counter_get_width(const struct pmt_counter *p)
@@ -3636,7 +3641,7 @@ int format_counters(PER_THREAD_PARAMS)
 		if (p->gfx_rc6_ms == -1) {	/* detect GFX counter reset */
 			outp += sprintf(outp, "%s**.**", (printed++ ? delim : ""));
 		} else {
-			outp += sprintf(outp, "%s%.2f", (printed++ ? delim : ""), p->gfx_rc6_ms / 10.0 / interval_float);
+			outp += sprintf(outp, "%s%.2f", (printed++ ? delim : ""), pct(p->gfx_rc6_ms / 10.0, interval_float));
 		}
 	}
 
@@ -3653,7 +3658,7 @@ int format_counters(PER_THREAD_PARAMS)
 		if (p->sam_mc6_ms == -1) {	/* detect GFX counter reset */
 			outp += sprintf(outp, "%s**.**", (printed++ ? delim : ""));
 		} else {
-			outp += sprintf(outp, "%s%.2f", (printed++ ? delim : ""), p->sam_mc6_ms / 10.0 / interval_float);
+			outp += sprintf(outp, "%s%.2f", (printed++ ? delim : ""), pct(p->sam_mc6_ms / 10.0, interval_float));
 		}
 	}
 
@@ -5915,7 +5920,7 @@ void free_fd_msr(void)
 	if (!msr_counter_info)
 		return;
 
-	for (int cpu = 0; cpu < topo.max_cpu_num; ++cpu) {
+	for (int cpu = 0; cpu <= topo.max_cpu_num; ++cpu) {
 		if (msr_counter_info[cpu].fd_perf != -1)
 			close(msr_counter_info[cpu].fd_perf);
 	}
@@ -6311,33 +6316,35 @@ int for_all_proc_cpus(int (func) (int))
 
 #define PATH_EFFECTIVE_CPUS	"/sys/fs/cgroup/cpuset.cpus.effective"
 
-static char cpu_effective_str[1024];
+static char *cpu_effective_str;
 
 static int update_effective_str(bool startup)
 {
 	FILE *fp;
-	char *pos;
-	char buf[1024];
+	char *buf = NULL;
+	size_t size = 0;
 	int ret;
 
-	if (cpu_effective_str[0] == '\0' && !startup)
+	if (!cpu_effective_str && !startup)
 		return 0;
 
 	fp = fopen(PATH_EFFECTIVE_CPUS, "r");
 	if (!fp)
 		return 0;
 
-	pos = fgets(buf, 1024, fp);
-	if (!pos)
+	if (getline(&buf, &size, fp) < 0)
 		err(1, "%s: file read failed", PATH_EFFECTIVE_CPUS);
 
 	fclose(fp);
 
-	ret = strncmp(cpu_effective_str, buf, 1024);
-	if (!ret)
+	ret = cpu_effective_str ? strcmp(cpu_effective_str, buf) : 1;
+	if (!ret) {
+		free(buf);
 		return 0;
+	}
 
-	strncpy(cpu_effective_str, buf, 1024);
+	free(cpu_effective_str);
+	cpu_effective_str = buf;
 	return 1;
 }
 
@@ -8529,33 +8536,37 @@ int add_rapl_perf_counter(int cpu, struct rapl_counter_info_t *rci, const struct
 	return ret;
 }
 
-char cpuset_buf[1024];
-int initialize_cpu_set_from_sysfs(cpu_set_t *cpu_set, char *sysfs_path, char *sysfs_file)
+int initialize_cpu_set_from_sysfs(cpu_set_t *cpu_set, const char *sysfs_path,
+				  const char *sysfs_file)
 {
 	FILE *fp;
+	char *cpuset_buf = NULL;
+	size_t size = 0;
 	char path[128];
+	int ret = -1;
 
-	if (snprintf(path, 128, "%s/%s", sysfs_path, sysfs_file) > 128)
+	if (snprintf(path, sizeof(path), "%s/%s", sysfs_path, sysfs_file) >= (int)sizeof(path))
 		err(-1, "%s %s", sysfs_path, sysfs_file);
 
 	fp = fopen(path, "r");
 	if (!fp) {
 		warn("open %s", path);
 		return -1;
 	}
-	if (fread(cpuset_buf, sizeof(char), 1024, fp) == 0) {
+	if (getline(&cpuset_buf, &size, fp) < 0) {
 		warn("read %s", sysfs_path);
-		goto err;
+		goto out;
 	}
 	if (parse_cpu_str(cpuset_buf, cpu_set, cpu_possible_setsize)) {
-		warnx("%s: cpu str malformat %s\n", sysfs_path, cpu_effective_str);
-		goto err;
+		warnx("%s: cpu str malformat %s\n", sysfs_path, cpuset_buf);
+		goto out;
 	}
-	return 0;
+	ret = 0;
 
-err:
+out:
 	fclose(fp);
-	return -1;
+	free(cpuset_buf);
+	return ret;
 }
 
 void print_cpu_se
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`close`
- 外部函数：`closedir`
- 外部函数：`cpu_is_not_allowed`
- 外部函数：`err`
- 外部函数：`fclose`
- 外部函数：`fgets`
- 外部函数：`fopen`
- 外部函数：`free`
- 外部函数：`initialize_cpu_set_from_sysfs`
- 外部函数：`scandir`
- 外部函数：`sprintf`
- 外部函数：`sscanf`
- 外部函数：`strncmp`
- 外部函数：`strncpy`
- 外部函数：`strtoul`
- 大写宏：`BIC`
- 大写宏：`BIC_L2_HIT`
- 大写宏：`BIC_L2_MRPS`
- 大写宏：`COUNTER_CYCLES`
- 大写宏：`COUNTER_ITEMS`
- 大写宏：`COUNTER_K2M`
- 大写宏：`COUNTER_NUMBER`
- 大写宏：`COUNTER_SECONDS`
- 大写宏：`COUNTER_USEC`
- 大写宏：`CPU_CLR`
- 大写宏：`CPU_ISSET_S`
- 大写宏：`CPU_SET`
- 大写宏：`CPU_SET_S`
- 大写宏：`FILE`
- 大写宏：`FORMAT_AVERAGE`
- 大写宏：`FORMAT_DELTA`
- 大写宏：`FORMAT_PERCENT`
- 大写宏：`FORMAT_RAW`
- 大写宏：`GFX`
- 大写宏：`GUID`
- 大写宏：`MMIO`
- 大写宏：`NULL`
- 大写宏：`PMT`
- 大写宏：`PMT_TYPE_RAW`
- 大写宏：`PMT_TYPE_TCORE_CLOCK`
- 大写宏：`PMT_TYPE_XTAL_TIME`
- 大写宏：`SCOPE_CORE`
- 大写宏：`SCOPE_CPU`
- 大写宏：`SCOPE_PACKAGE`
- 外部类型：`Pointer`
- 外部类型：`This`
- 外部类型：`cpu_set_t`
- 外部类型：`dirent`
- 外部类型：`sysfs_path`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-3bd918d74d` → 本草稿移入 `cases/defect/auto-linux-3bd918d74d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
