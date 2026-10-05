# auto-linux-f4fc9b9285

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a27611f8994c9a4772156432badb533af33fd32d](https://github.com/torvalds/linux/commit/a27611f8994c9a4772156432badb533af33fd32d) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 19 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a27611f8994c9a4772156432badb533af33fd32d (https://github.com/torvalds/linux/commit/a27611f8994c9a4772156432badb533af33fd32d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 1292；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a27611f8994c9a4772156432badb533af33fd32d Merge tag 'perf-urgent-2026-10-04' of git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1275,6 +1275,8 @@ static int trace_imc_prepare_sample(struct trace_imc_data *mem,
 				    struct perf_event_header *header,
 				    struct perf_event *event)
 {
+	u16 misc = 0;
+
 	/* Sanity checks for a valid record */
 	if (be64_to_cpu(READ_ONCE(mem->tb1)) > *prev_tb)
 		*prev_tb = be64_to_cpu(READ_ONCE(mem->tb1));
@@ -1289,35 +1291,34 @@ static int trace_imc_prepare_sample(struct trace_imc_data *mem,
 	data->ip =  be64_to_cpu(READ_ONCE(mem->ip));
 	data->period = event->hw.last_period;
 
-	header->type = PERF_RECORD_SAMPLE;
-	header->size = sizeof(*header) + event->header_size;
-	header->misc = 0;
-
 	if (cpu_has_feature(CPU_FTR_ARCH_31)) {
 		switch (IMC_TRACE_RECORD_VAL_HVPR(be64_to_cpu(READ_ONCE(mem->val)))) {
 		case 0:/* when MSR HV and PR not set in the trace-record */
-			header->misc |= PERF_RECORD_MISC_GUEST_KERNEL;
+			misc |= PERF_RECORD_MISC_GUEST_KERNEL;
 			break;
 		case 1: /* MSR HV is 0 and PR is 1 */
-			header->misc |= PERF_RECORD_MISC_GUEST_USER;
+			misc |= PERF_RECORD_MISC_GUEST_USER;
 			break;
 		case 2: /* MSR HV is 1 and PR is 0 */
-			header->misc |= PERF_RECORD_MISC_KERNEL;
+			misc |= PERF_RECORD_MISC_KERNEL;
 			break;
 		case 3: /* MSR HV is 1 and PR is 1 */
-			header->misc |= PERF_RECORD_MISC_USER;
+			misc |= PERF_RECORD_MISC_USER;
 			break;
 		default:
 			pr_info("IMC: Unable to set the flag based on MSR bits\n");
 			break;
 		}
 	} else {
 		if (is_kernel_addr(data->ip))
-			header->misc |= PERF_RECORD_MISC_KERNEL;
+			misc |= PERF_RECORD_MISC_KERNEL;
 		else
-			header->misc |= PERF_RECORD_MISC_USER;
+			misc |= PERF_RECORD_MISC_USER;
 	}
-	perf_event_header__init_id(header, data, event);
+	perf_event_header__init(header, data,
+				PERF_RECORD_SAMPLE, misc,
+				sizeof(*header) + event->header_size,
+				event);
 
 	return 0;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`be64_to_cpu`
- 外部函数：`cpu_has_feature`
- 外部函数：`is_kernel_addr`
- 外部函数：`perf_event_header__init_id`
- 外部函数：`pr_info`
- 大写宏：`CPU_FTR_ARCH_31`
- 大写宏：`IMC`
- 大写宏：`IMC_TRACE_RECORD_VAL_HVPR`
- 大写宏：`MSR`
- 大写宏：`PERF_RECORD_MISC_GUEST_KERNEL`
- 大写宏：`PERF_RECORD_MISC_GUEST_USER`
- 大写宏：`PERF_RECORD_MISC_KERNEL`
- 大写宏：`PERF_RECORD_MISC_USER`
- 大写宏：`PERF_RECORD_SAMPLE`
- 大写宏：`READ_ONCE`
- 外部类型：`Sanity`
- 外部类型：`Unable`
- 外部类型：`perf_event`
- 外部类型：`perf_event_header`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-f4fc9b9285` → 本草稿移入 `cases/defect/auto-linux-f4fc9b9285/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
