# auto-linux-e425fabd46

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7be4bc52fd9e0e55dc9a2e0232b89427d2c4b614](https://github.com/torvalds/linux/commit/7be4bc52fd9e0e55dc9a2e0232b89427d2c4b614) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 93 |
| 编译错误数（gcc syntax-only） | 68（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7be4bc52fd9e0e55dc9a2e0232b89427d2c4b614 (https://github.com/torvalds/linux/commit/7be4bc52fd9e0e55dc9a2e0232b89427d2c4b614)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2617；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7be4bc52fd9e0e55dc9a2e0232b89427d2c4b614 Merge tag 'driver-core-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/driver-core/driver-core :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2614,17 +2614,17 @@ static DEVICE_BOOL_ATTR(dont_log_ce, 0644, mca_cfg.dont_log_ce);
 static DEVICE_BOOL_ATTR(print_all, 0644, mca_cfg.print_all);
 
 static struct dev_ext_attribute dev_attr_check_interval = {
-	__ATTR(check_interval, 0644, device_show_int, store_int_with_restart),
+	__DEVICE_ATTR(check_interval, 0644, device_show_int, store_int_with_restart),
 	&check_interval
 };
 
 static struct dev_ext_attribute dev_attr_ignore_ce = {
-	__ATTR(ignore_ce, 0644, device_show_bool, set_ignore_ce),
+	__DEVICE_ATTR(ignore_ce, 0644, device_show_bool, set_ignore_ce),
 	&mca_cfg.ignore_ce
 };
 
 static struct dev_ext_attribute dev_attr_cmci_disabled = {
-	__ATTR(cmci_disabled, 0644, device_show_bool, set_cmci_disabled),
+	__DEVICE_ATTR(cmci_disabled, 0644, device_show_bool, set_cmci_disabled),
 	&mca_cfg.cmci_disabled
 };
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__ATTR`
- 外部函数：`__this_cpu_read`
- 外部函数：`__this_cpu_write`
- 外部函数：`barrier`
- 外部函数：`bitmap_fill`
- 外部函数：`clear_bank`
- 外部函数：`cmci_clear`
- 外部函数：`cmci_recheck`
- 外部函数：`cmci_reenable`
- 外部函数：`cpu_has`
- 外部函数：`cr4_set_bits`
- 外部函数：`device_store_ulong`
- 外部函数：`for_each_online_cpu`
- 外部函数：`inc_irq_stat`
- 外部函数：`kstrtou64`
- 外部函数：`local_irq_restore`
- 外部函数：`local_irq_save`
- 外部函数：`max`
- 外部函数：`mc_poll_banks`
- 外部函数：`mca_msr_reg`
- 外部函数：`mce_gather_info`
- 外部函数：`mce_gen_pool_add`
- 外部函数：`mce_gen_pool_empty`
- 外部函数：`mce_get_storm_mode`
- 外部函数：`mce_log`
- 外部函数：`mce_rdmsrq`
- 外部函数：`mce_read_aux`
- 外部函数：`mce_severity`
- 外部函数：`mce_track_storm`
- 外部函数：`mce_usable_address`
- 外部函数：`min`
- 外部函数：`mod_timer`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`on_each_cpu`
- 外部函数：`per_cpu`
- 外部函数：`raw_cpu_ptr`
- 外部函数：`rdmsrq`
- 外部函数：`rdtsc`
- 外部函数：`round_jiffies`
- 外部函数：`round_jiffies_relative`
- 外部函数：`schedule_work`
- 外部函数：`should_log_poll_error`
- 外部函数：`sync_core`
- 外部函数：`test_bit`
- 外部函数：`this_cpu_ptr`
- 外部函数：`this_cpu_read`
- 外部函数：`this_cpu_write`
- 外部函数：`time_before`
- 外部函数：`timer_delete_sync`
- 外部函数：`timer_pending`
- 外部函数：`timer_setup`
- 外部函数：`wrmsrq`
- 大写宏：`DEVICE_BOOL_ATTR`
- 大写宏：`EINVAL`
- 大写宏：`MAX_NR_BANKS`
- 大写宏：`MCA_CTL`
- 大写宏：`MCA_STATUS`
- 大写宏：`MCE`
- 大写宏：`MCE_POLL`
- 大写宏：`MCG_CTL_P`
- 大写宏：`MCG_STATUS`
- 大写宏：`MCI_STATUS_VAL`
- 大写宏：`MCP_QUEUE_LOG`
- 大写宏：`MCP_TIMESTAMP`
- 大写宏：`MCP_UC`
- 大写宏：`MSR_IA32_MCG_CAP`
- 大写宏：`MSR_IA32_MCG_CTL`
- 大写宏：`NULL`
- 大写宏：`TIMER_PINNED`
- 大写宏：`WARN_ON`
- 大写宏：`X86_CR4_MCE`
- 大写宏：`X86_FEATURE_MCA`
- 大写宏：`X86_FEATURE_MCE`
- 外部类型：`Alert`
- 外部类型：`Don`
- 外部类型：`If`
- 外部类型：`Log`
- 外部类型：`No`
- 外部类型：`That`
- 外部类型：`Update`
- 外部类型：`Valid`
- 外部类型：`Verify`
- 外部类型：`cpu_t`
- 外部类型：`cpuinfo_x86`
- 外部类型：`dev_ext_attribute`
- 外部类型：`device`
- 外部类型：`device_attribute`
- 外部类型：`mce`
- 外部类型：`mce_bank`
- 外部类型：`mce_banks_t`
- 外部类型：`mce_hw_err`
- 外部类型：`mcp_flags`
- 外部类型：`size_t`
- 外部类型：`ssize_t`
- 外部类型：`timer_list`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e425fabd46` → 本草稿移入 `cases/defect/auto-linux-e425fabd46/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
