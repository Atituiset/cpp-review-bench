# auto-linux-3c7c9b378b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#25d576ed11108470d0999054265108400db1b881](https://github.com/torvalds/linux/commit/25d576ed11108470d0999054265108400db1b881) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 103 |
| 编译错误数（gcc syntax-only） | 102（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #25d576ed11108470d0999054265108400db1b881 (https://github.com/torvalds/linux/commit/25d576ed11108470d0999054265108400db1b881)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2933；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 25d576ed11108470d0999054265108400db1b881 Merge tag 'char-misc-7.3-rc6' of git://git.kernel.org/pub/scm/linux/kernel/git/gregkh/char-misc :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2930,8 +2930,9 @@ static int binder_proc_transaction(struct binder_transaction *t,
 		t_outdated->buffer = NULL;
 		buffer->transaction = NULL;
 		trace_binder_transaction_update_buffer_release(buffer);
-		binder_release_entire_buffer(proc, NULL, buffer, false);
+		binder_release_entire_buffer(proc, NULL, buffer, true);
 		binder_alloc_free_buf(&proc->alloc, buffer);
+		binder_free_txn_fixups(t_outdated);
 		kfree(t_outdated);
 		binder_stats_deleted(BINDER_STAT_TRANSACTION);
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__acquire`
- 外部函数：`__release`
- 外部函数：`_binder_inner_proc_lock`
- 外部函数：`_binder_inner_proc_unlock`
- 外部函数：`_binder_node_inner_lock`
- 外部函数：`_binder_node_inner_unlock`
- 外部函数：`_binder_proc_lock`
- 外部函数：`_binder_proc_unlock`
- 外部函数：`assert_spin_locked`
- 外部函数：`atomic_inc`
- 外部函数：`binder_alloc_copy_from_buffer`
- 外部函数：`binder_alloc_free_buf`
- 外部函数：`binder_cleanup_ref_olocked`
- 外部函数：`binder_debug`
- 外部函数：`binder_enqueue_work_ilocked`
- 外部函数：`binder_inc_node`
- 外部函数：`binder_inner_proc_lock`
- 外部函数：`binder_inner_proc_unlock`
- 外部函数：`binder_node_inner_lock`
- 外部函数：`binder_node_inner_unlock`
- 外部函数：`binder_proc_unlock`
- 外部函数：`binder_select_thread_ilocked`
- 外部函数：`binder_stats_deleted`
- 外部函数：`binder_user_error`
- 外部函数：`binder_wakeup_thread_ilocked`
- 外部函数：`bitmap_zalloc`
- 外部函数：`container_of`
- 外部函数：`copy_from_user`
- 外部函数：`dbitmap_shrink`
- 外部函数：`dbitmap_shrink_nbits`
- 外部函数：`fds`
- 外部函数：`file_close_fd`
- 外部函数：`filp_close`
- 外部函数：`fput`
- 外部函数：`get_file`
- 外部函数：`hlist_del`
- 外部函数：`hlist_empty`
- 外部函数：`init_task_work`
- 外部函数：`kfree`
- 外部函数：`kzalloc_obj`
- 外部函数：`list_add_tail`
- 外部函数：`list_del_init`
- 外部函数：`list_empty`
- 外部函数：`min_t`
- 外部函数：`pr_err`
- 外部函数：`pr_info_ratelimited`
- 外部函数：`rb_entry`
- 外部函数：`rb_erase`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`task_work_add`
- 外部函数：`to_binder_fd_array_object`
- 外部函数：`trace_binder_transaction_update_buffer_release`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`ALIGN`
- 大写宏：`BINDER_DEBUG_INTERNAL_REFS`
- 大写宏：`BINDER_DEBUG_TRANSACTION`
- 大写宏：`BINDER_STAT_NODE`
- 大写宏：`BINDER_STAT_TRANSACTION`
- 大写宏：`BINDER_TYPE_BINDER`
- 大写宏：`BINDER_TYPE_FD`
- 大写宏：`BINDER_TYPE_FDA`
- 大写宏：`BINDER_TYPE_HANDLE`
- 大写宏：`BINDER_TYPE_PTR`
- 大写宏：`BINDER_TYPE_WEAK_BINDER`
- 大写宏：`BINDER_TYPE_WEAK_HANDLE`
- 大写宏：`BUG_ON`
- 大写宏：`EINVAL`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IS_ALIGNED`
- 大写宏：`NULL`
- 大写宏：`SIZE_MAX`
- 大写宏：`TWA_RESUME`
- 大写宏：`WARN_ON`
- 外部类型：`Call`
- 外部类型：`Calling`
- 外部类型：`Convert`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`Need`
- 外部类型：`No`
- 外部类型：`Nothing`
- 外部类型：`Ok`
- 外部类型：`The`
- 外部类型：`binder_buffer`
- 外部类型：`binder_buffer_object`
- 外部类型：`binder_fd_array_object`
- 外部类型：`binder_fd_object`
- 外部类型：`binder_node`
- 外部类型：`binder_object`
- 外部类型：`binder_object_header`
- 外部类型：`binder_proc`
- 外部类型：`binder_ref`
- 外部类型：`binder_ref_data`
- 外部类型：`binder_size_t`
- 外部类型：`binder_stat_types`
- 外部类型：`binder_thread`
- 外部类型：`binder_uintptr_t`
- 外部类型：`binder_work`
- 外部类型：`callback_head`
- 外部类型：`file`
- 外部类型：`flat_binder_object`
- 外部类型：`list_head`
- 外部类型：`rb_node`
- 外部类型：`size_t`
- 外部类型：`uint32_t`
- 外部类型：`va_format`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-3c7c9b378b` → 本草稿移入 `cases/defect/auto-linux-3c7c9b378b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
