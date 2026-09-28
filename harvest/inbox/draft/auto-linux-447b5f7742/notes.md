# auto-linux-447b5f7742

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#efb44d93a620c294c049db37c810c8ab7ee1122d](https://github.com/torvalds/linux/commit/efb44d93a620c294c049db37c810c8ab7ee1122d) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 31 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #efb44d93a620c294c049db37c810c8ab7ee1122d (https://github.com/torvalds/linux/commit/efb44d93a620c294c049db37c810c8ab7ee1122d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 324；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT efb44d93a620c294c049db37c810c8ab7ee1122d Merge tag 'x86-urgent-2026-09-27' of git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -74,6 +74,14 @@ int svsm_perform_call_protocol(struct svsm_call *call)
 
 	flags = native_local_irq_save();
 
+	/*
+	 * 'caa' is a per-CPU variable. To avoid using a stale or incorrect
+	 * 'caa' if the task is preempted or migrated to another CPU after it
+	 * is fetched, always fetch 'caa' and then issue the SVSM call with
+	 * interrupts disabled. This ensures the correct 'caa' is used.
+	 */
+	call->caa = svsm_get_caa();
+
 	ghcb = __sev_get_ghcb(&state);
 
 	do {
@@ -321,7 +329,6 @@ int snp_svsm_vtpm_send_command(u8 *buffer)
 {
 	struct svsm_call call = {};
 
-	call.caa = svsm_get_caa();
 	call.rax = SVSM_VTPM_CALL(SVSM_VTPM_CMD);
 	call.rcx = __pa(buffer);
 
@@ -345,7 +352,6 @@ bool snp_svsm_vtpm_probe(void)
 	if (!snp_vmpl)
 		return false;
 
-	call.caa = svsm_get_caa();
 	call.rax = SVSM_VTPM_CALL(SVSM_VTPM_QUERY);
 
 	if (svsm_perform_call_protocol(&call))
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__pa`
- 外部函数：`__pi_svsm_perform_msr_protocol`
- 外部函数：`__sev_get_ghcb`
- 外部函数：`__sev_put_ghcb`
- 外部函数：`ghcb_set_sw_exit_code`
- 外部函数：`ghcb_set_sw_exit_info_1`
- 外部函数：`ghcb_set_sw_exit_info_2`
- 外部函数：`native_local_irq_restore`
- 外部函数：`native_local_irq_save`
- 外部函数：`sev_es_wr_ghcb_msr`
- 外部函数：`svsm_get_caa`
- 外部函数：`svsm_issue_call`
- 外部函数：`svsm_process_result_codes`
- 外部函数：`vc_forward_exception`
- 外部函数：`vc_ghcb_invalidate`
- 外部函数：`verify_exception_info`
- 大写宏：`EAGAIN`
- 大写宏：`EINVAL`
- 大写宏：`ES_EXCEPTION`
- 大写宏：`ES_OK`
- 大写宏：`GHCB_DEFAULT_USAGE`
- 大写宏：`SVM_VMGEXIT_SNP_RUN_VMPL`
- 大写宏：`SVSM_VTPM_CALL`
- 大写宏：`SVSM_VTPM_CMD`
- 大写宏：`SVSM_VTPM_QUERY`
- 外部类型：`Fill`
- 外部类型：`This`
- 外部类型：`es_em_ctxt`
- 外部类型：`ghcb`
- 外部类型：`ghcb_state`
- 外部类型：`svsm_call`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-447b5f7742` → 本草稿移入 `cases/defect/auto-linux-447b5f7742/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
