# auto-linux-483c83bccc

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#0d508d4709837bcc88e314c0533a42d3c55b31e3](https://github.com/torvalds/linux/commit/0d508d4709837bcc88e314c0533a42d3c55b31e3) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 17 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0d508d4709837bcc88e314c0533a42d3c55b31e3 (https://github.com/torvalds/linux/commit/0d508d4709837bcc88e314c0533a42d3c55b31e3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 110；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0d508d4709837bcc88e314c0533a42d3c55b31e3 Merge tag 'audit-pr-20261016' of git://git.kernel.org/pub/scm/linux/kernel/git/pcmoore/audit :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -107,7 +107,7 @@ struct audit_aux_data_pids {
 
 struct audit_aux_data_bprm_fcaps {
 	struct audit_aux_data	d;
-	struct audit_cap_data	fcap;
+	struct audit_file_caps	fcap;
 	unsigned int		fcap_ver;
 	struct audit_cap_data	old_pcap;
 	struct audit_cap_data	new_pcap;
@@ -2601,6 +2601,8 @@ void __audit_bprm(struct linux_binprm *bprm)
 {
 	struct audit_context *context = audit_context();
 
+	/* clear proctitle in audit context to allow replacement */
+	audit_proctitle_free(context);
 	context->type = AUDIT_EXECVE;
 	context->execve.argc = bprm->argc;
 }
@@ -2883,6 +2885,19 @@ void __audit_log_nfcfg(const char *name, u8 af, unsigned int nentries,
 }
 EXPORT_SYMBOL_GPL(__audit_log_nfcfg);
 
+void __audit_log_fsopen(const char *fs_name)
+{
+	struct audit_buffer *ab;
+
+	ab = audit_log_start(audit_context(), GFP_KERNEL, AUDIT_FSOPEN);
+	if (!ab)
+		return;
+
+	audit_log_format(ab, "fs_name=");
+	audit_log_untrustedstring(ab, fs_name);
+	audit_log_end(ab);
+}
+
 static void audit_log_task(struct audit_buffer *ab)
 {
 	kuid_t auid, uid;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`audit_context`
- 外部函数：`audit_log_end`
- 外部函数：`audit_log_format`
- 外部函数：`audit_log_start`
- 外部函数：`audit_log_task_context`
- 外部函数：`audit_log_untrustedstring`
- 外部函数：`get_task_comm`
- 外部函数：`task_tgid_nr`
- 大写宏：`AUDIT_EXECVE`
- 大写宏：`AUDIT_NETFILTER_CFG`
- 大写宏：`EXPORT_SYMBOL_GPL`
- 外部类型：`audit_buffer`
- 外部类型：`audit_cap_data`
- 外部类型：`audit_context`
- 外部类型：`audit_nfcfgop`
- 外部类型：`gfp_t`
- 外部类型：`kuid_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-483c83bccc` → 本草稿移入 `cases/defect/auto-linux-483c83bccc/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
