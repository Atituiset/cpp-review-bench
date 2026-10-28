# auto-linux-8008fe6014

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#59a02493f3b68fd0ad68c661393f680bcca5a85b](https://github.com/torvalds/linux/commit/59a02493f3b68fd0ad68c661393f680bcca5a85b) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 56 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #59a02493f3b68fd0ad68c661393f680bcca5a85b (https://github.com/torvalds/linux/commit/59a02493f3b68fd0ad68c661393f680bcca5a85b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 54（原始 PR diff 行 2005；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 59a02493f3b68fd0ad68c661393f680bcca5a85b Merge tag 'trace-v7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/trace/linux-trace :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -72,6 +72,7 @@ enum filter_pred_fn {
 	FILTER_PRED_FN_CPUMASK,
 	FILTER_PRED_FN_CPUMASK_CPU,
 	FILTER_PRED_FN_FUNCTION,
+	FILTER_PRED_FN_WITHIN,
 	FILTER_PRED_FN_,
 	FILTER_PRED_TEST_VISITED,
 };
@@ -117,6 +118,7 @@ struct filter_pred {
 	C(IP_FIELD_ONLY,	"Only 'ip' field is supported for function trace"), \
 	C(INVALID_VALUE,	"Invalid value (did you forget quotes)?"), \
 	C(NO_FUNCTION,		"Function not found"),			\
+	C(GLOB_WITHIN,		"'within' filter cannot take glob expressions"), \
 	C(ERRNO,		"Error"),				\
 	C(NO_FILTER,		"No filter found")
 
@@ -595,6 +597,12 @@ predicate_parse(const char *str, int nr_parens, int nr_preds,
 			top--;					/* #12 */
 		}
 	}
+	/* Only the inner loop can complete a non-empty expression. */
+	if (N) {
+		ret = -EINVAL;
+		parse_error(pe, FILT_ERR_INVALID_FILTER, ptr - str);
+		goto out_free;
+	}
  out:
 	if (top != op_stack) {
 		/* Too many '(' */
@@ -1008,6 +1016,22 @@ static int filter_pred_function(struct filter_pred *pred, void *event)
 	return pred->op == OP_EQ ? ret : !ret;
 }
 
+/* Filter predicate for within. */
+static int filter_pred_within(struct filter_pred *pred, void *event)
+{
+#ifdef CONFIG_STACKTRACE
+	unsigned long entries[16];
+	unsigned int nr_entries;
+	int i;
+
+	nr_entries = stack_trace_save(entries, ARRAY_SIZE(entries), 0);
+	for (i = 0; i < nr_entries; i++)
+		if (pred->val <= entries[i] && entries[i] < pred->val2)
+			return !pred->not;
+#endif
+	return pred->not;
+}
+
 /*
  * regex_match_foo - Basic regex callbacks
  *
@@ -1617,6 +1641,8 @@ static int filter_pred_fn_call(struct filter_pred *pred, void *event)
 		return filter_pred_cpumask_cpu(pred, event);
 	case FILTER_PRED_FN_FUNCTION:
 		return filter_pred_function(pred, event);
+	case FILTER_PRED_FN_WITHIN:
+		return filter_pred_within(pred, event);
 	case FILTER_PRED_TEST_VISITED:
 		return test_pred_visited_fn(pred, event);
 	default:
@@ -2002,10 +2028,30 @@ static int parse_pred(const char *str, void *data,
 
 		} else if (field->filter_type == FILTER_DYN_STRING) {
 			pred->fn_num = FILTER_PRED_FN_STRLOC;
-		} else if (field->filter_type == FILTER_RDYN_STRING)
+		} else if (field->filter_type == FILTER_RDYN_STRING) {
 			pred->fn_num = FILTER_PRED_FN_STRRELLOC;
-		else {
+		} else if (field->filter_type == FILTER_WITHIN) {
+			unsigned long func;
 
+			if (op == OP_GLOB) {
+				parse_error(pe, FILT_ERR_GLOB_WITHIN, pos + i);
+				goto err_free;
+			}
+
+			pred->fn_num = FILTER_PRED_FN_WITHIN;
+			func = kallsyms_lookup_name(pred->regex->pattern);
+			if (!func) {
+				parse_error(pe, FILT_ERR_NO_FUNCTION, pos + i);
+				goto err_free;
+			}
+			/* Now find the function start and end address */
+			if (!kallsyms_lookup_size_offset(func, &size, &offset)) {
+				parse_error(pe, FILT_ERR_NO_FUNCTION, pos + i);
+				goto err_free;
+			}
+			pred->val = func - offset;
+			pred->val2 = pred->val + size;
+		} else {
 			if (!ustring_per_cpu) {
 				/* Once allocated, keep it around for good */
 				ustring_per_cpu = alloc_percpu(struct ustring_buffer);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`alloc_percpu`
- 外部函数：`cpumask_nth`
- 外部函数：`cpumask_test_cpu`
- 外部函数：`value`
- 大写宏：`ERRNO`
- 大写宏：`FILTER_DYN_STRING`
- 大写宏：`FILTER_PRED_FN_`
- 大写宏：`FILTER_PRED_FN_16`
- 大写宏：`FILTER_PRED_FN_16_CPUMASK`
- 大写宏：`FILTER_PRED_FN_32`
- 大写宏：`FILTER_PRED_FN_32_CPUMASK`
- 大写宏：`FILTER_PRED_FN_64`
- 大写宏：`FILTER_PRED_FN_64_CPUMASK`
- 大写宏：`FILTER_PRED_FN_8`
- 大写宏：`FILTER_PRED_FN_8_CPUMASK`
- 大写宏：`FILTER_PRED_FN_COMM`
- 大写宏：`FILTER_PRED_FN_CPU`
- 大写宏：`FILTER_PRED_FN_CPUMASK`
- 大写宏：`FILTER_PRED_FN_CPUMASK_CPU`
- 大写宏：`FILTER_PRED_FN_CPU_CPUMASK`
- 大写宏：`FILTER_PRED_FN_FUNCTION`
- 大写宏：`FILTER_PRED_FN_NOP`
- 大写宏：`FILTER_PRED_FN_PCHAR`
- 大写宏：`FILTER_PRED_FN_PCHAR_USER`
- 大写宏：`FILTER_PRED_FN_S16`
- 大写宏：`FILTER_PRED_FN_S32`
- 大写宏：`FILTER_PRED_FN_S64`
- 大写宏：`FILTER_PRED_FN_S8`
- 大写宏：`FILTER_PRED_FN_STRING`
- 大写宏：`FILTER_PRED_FN_STRLOC`
- 大写宏：`FILTER_PRED_FN_STRRELLOC`
- 大写宏：`FILTER_PRED_FN_U16`
- 大写宏：`FILTER_PRED_FN_U32`
- 大写宏：`FILTER_PRED_FN_U64`
- 大写宏：`FILTER_PRED_FN_U8`
- 大写宏：`FILTER_PRED_TEST_VISITED`
- 大写宏：`FILTER_RDYN_STRING`
- 大写宏：`INVALID_VALUE`
- 大写宏：`IP_FIELD_ONLY`
- 大写宏：`NO_FILTER`
- 大写宏：`NO_FUNCTION`
- 大写宏：`OP_BAND`
- 大写宏：`OP_EQ`
- 大写宏：`OP_NE`
- 外部类型：`Basic`
- 外部类型：`Error`
- 外部类型：`Function`
- 外部类型：`Invalid`
- 外部类型：`No`
- 外部类型：`Once`
- 外部类型：`Only`
- 外部类型：`Too`
- 外部类型：`cpumask`
- 外部类型：`ftrace_event_field`
- 外部类型：`regex`
- 外部类型：`ustring_buffer`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-8008fe6014` → 本草稿移入 `cases/defect/auto-linux-8008fe6014/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
