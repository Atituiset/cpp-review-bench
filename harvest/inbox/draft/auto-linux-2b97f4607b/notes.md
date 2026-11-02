# auto-linux-2b97f4607b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#cbaa26615075e26ab54e0e8afaa10860867f75fe](https://github.com/torvalds/linux/commit/cbaa26615075e26ab54e0e8afaa10860867f75fe) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 34 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cbaa26615075e26ab54e0e8afaa10860867f75fe (https://github.com/torvalds/linux/commit/cbaa26615075e26ab54e0e8afaa10860867f75fe)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1088；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cbaa26615075e26ab54e0e8afaa10860867f75fe Merge tag 'bootconfig-fixes-v7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/trace/linux-trace :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1085,12 +1085,12 @@ void start_kernel(void)
 				  __stop___param - __start___param,
 				  -1, -1, NULL, &unknown_bootoption);
 	print_unknown_bootoptions();
-	if (!IS_ERR_OR_NULL(after_dashes))
-		parse_args("Setting init args", after_dashes, NULL, 0, -1, -1,
-			   NULL, set_init_arg);
 	if (extra_init_args)
 		parse_args("Setting extra init args", extra_init_args,
 			   NULL, 0, -1, -1, NULL, set_init_arg);
+	if (!IS_ERR_OR_NULL(after_dashes))
+		parse_args("Setting init args", after_dashes, NULL, 0, -1, -1,
+			   NULL, set_init_arg);
 
 	/* Architectural and non-timekeeping rng init, before allocator init */
 	random_init_early(command_line);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`kdump`
- 外部函数：`memblock_alloc`
- 外部函数：`memblock_free`
- 外部函数：`memmove`
- 外部函数：`parameqn`
- 外部函数：`parse_args`
- 外部函数：`pr_err`
- 外部函数：`pr_notice`
- 外部函数：`random_init_early`
- 外部函数：`setup_func`
- 外部函数：`sprintf`
- 外部函数：`strlen`
- 外部函数：`strnchr`
- 外部函数：`strncmp`
- 外部函数：`strstarts`
- 外部函数：`sysctl_is_alias`
- 大写宏：`BOOT_IMAGE`
- 大写宏：`BUG`
- 大写宏：`CONFIG_INIT_ENV_ARG_LIMIT`
- 大写宏：`IS_ERR_OR_NULL`
- 大写宏：`LILO`
- 大写宏：`NULL`
- 大写宏：`SMP_CACHE_BYTES`
- 外部类型：`Already`
- 外部类型：`Architectural`
- 外部类型：`Command`
- 外部类型：`Determine`
- 外部类型：`Environment`
- 外部类型：`Failed`
- 外部类型：`Grub`
- 外部类型：`Handle`
- 外部类型：`Keep`
- 外部类型：`Needs`
- 外部类型：`Setting`
- 外部类型：`Start`
- 外部类型：`Unknown`
- 外部类型：`Unused`
- 外部类型：`Well`
- 外部类型：`obs_kernel_param`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-2b97f4607b` → 本草稿移入 `cases/defect/auto-linux-2b97f4607b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
