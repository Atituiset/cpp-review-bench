# auto-linux-1f5da7f2b6

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd](https://github.com/torvalds/linux/commit/4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 168 |
| 编译错误数（gcc syntax-only） | 241（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd (https://github.com/torvalds/linux/commit/4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1325；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd Merge tag 'mm-nonmm-hotfixes-stable-2026-11-18-00-06' of git://git.kernel.org/pub/scm/linux/kernel/git/akpm/mm :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1318,15 +1318,16 @@ static void ocfs2_clear_inode(struct inode *inode)
 	oi->ip_blkno = 0ULL;
 
 	/*
-	 * ip_jinode is used to track txns against this inode. We ensure that
-	 * the journal is flushed before journal shutdown. Thus it is safe to
-	 * have inodes get cleaned up after journal shutdown.
+	 * ip_jinode is used to track txns against this inode. The journal is
+	 * flushed before it is shut down, so an inode cleaned up after that
+	 * has nothing left to release. Hold the shutdown lock so a journal
+	 * found here stays allocated until the release has finished.
 	 */
-	if (!osb->journal)
-		return;
-
-	jbd2_journal_release_jbd_inode(osb->journal->j_journal,
-				       &oi->ip_jinode);
+	down_read(&osb->journal_shutdown_sem);
+	if (osb->journal)
+		jbd2_journal_release_jbd_inode(osb->journal->j_journal,
+					       &oi->ip_jinode);
+	up_read(&osb->journal_shutdown_sem);
 }
 
 void ocfs2_evict_inode(struct inode *inode)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`anyway`
- 外部函数：`brelse`
- 外部函数：`clear_inode`
- 外部函数：`cpu_to_le16`
- 外部函数：`cpu_to_le32`
- 外部函数：`cpu_to_le64`
- 外部函数：`delete_inode`
- 外部函数：`down_trylock`
- 外部函数：`down_write_trylock`
- 外部函数：`dquot_drop`
- 外部函数：`dquot_free_inode`
- 外部函数：`dquot_initialize`
- 外部函数：`filemap_write_and_wait`
- 外部函数：`i_gid_read`
- 外部函数：`i_size_read`
- 外部函数：`i_size_write`
- 外部函数：`i_uid_read`
- 外部函数：`inode_get_atime_nsec`
- 外部函数：`inode_get_atime_sec`
- 外部函数：`inode_get_ctime_nsec`
- 外部函数：`inode_get_ctime_sec`
- 外部函数：`inode_get_mtime_nsec`
- 外部函数：`inode_get_mtime_sec`
- 外部函数：`inode_lock`
- 外部函数：`inode_unlock`
- 外部函数：`iput`
- 外部函数：`is_bad_inode`
- 外部函数：`jbd2_journal_release_jbd_inode`
- 外部函数：`ktime_get_real_seconds`
- 外部函数：`le16_to_cpu`
- 外部函数：`le32_to_cpu`
- 外部函数：`le64_to_cpu`
- 外部函数：`list_empty`
- 外部函数：`llu`
- 外部函数：`lock`
- 外部函数：`metadata`
- 外部函数：`mlog`
- 外部函数：`mlog_bug_on_msg`
- 外部函数：`mlog_errno`
- 外部函数：`mutex_trylock`
- 外部函数：`mutex_unlock`
- 外部函数：`ocfs2_begin_ordered_truncate`
- 外部函数：`ocfs2_block_signals`
- 外部函数：`ocfs2_checkpoint_inode`
- 外部函数：`ocfs2_commit_trans`
- 外部函数：`ocfs2_commit_truncate`
- 外部函数：`ocfs2_drop_inode_locks`
- 外部函数：`ocfs2_dx_dir_truncate`
- 外部函数：`ocfs2_extent_map_trunc`
- 外部函数：`ocfs2_free_dinode`
- 外部函数：`ocfs2_get_inode_flags`
- 外部函数：`ocfs2_get_system_file_inode`
- 外部函数：`ocfs2_inode_lock`
- 外部函数：`ocfs2_inode_unlock`
- 外部函数：`ocfs2_journal_access_di`
- 外部函数：`ocfs2_journal_dirty`
- 外部函数：`ocfs2_lock_res_free`
- 外部函数：`ocfs2_mark_lockres_freeing`
- 外部函数：`ocfs2_metadata_cache_exit`
- 外部函数：`ocfs2_nfs_sync_lock`
- 外部函数：`ocfs2_nfs_sync_unlock`
- 外部函数：`ocfs2_node_map_test_bit`
- 外部函数：`ocfs2_open_unlock`
- 外部函数：`ocfs2_orphan_del`
- 外部函数：`ocfs2_quota_trans_credits`
- 外部函数：`ocfs2_read_locked_inode`
- 外部函数：`ocfs2_remove_from_cache`
- 外部函数：`ocfs2_remove_refcount_tree`
- 外部函数：`ocfs2_resv_discard`
- 外部函数：`ocfs2_resv_init_once`
- 外部函数：`ocfs2_set_links_count`
- 外部函数：`ocfs2_should_order_data`
- 外部函数：`ocfs2_start_trans`
- 外部函数：`ocfs2_stuff_meta_lvb`
- 外部函数：`ocfs2_try_open_lock`
- 外部函数：`ocfs2_unblock_signals`
- 外部函数：`ocfs2_update_inode_fsync_trans`
- 外部函数：`ocfs2_xattr_remove`
- 外部函数：`read_inode`
- 外部函数：`spin_is_locked`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`trace_ocfs2_check_orphan_recovery_state`
- 外部函数：`trace_ocfs2_cleanup_delete_inode`
- 外部函数：`trace_ocfs2_clear_inode`
- 外部函数：`trace_ocfs2_delete_inode`
- 外部函数：`trace_ocfs2_inode_is_valid_to_delete`
- 外部函数：`trace_ocfs2_mark_inode_dirty`
- 外部函数：`trace_ocfs2_query_inode_wipe_begin`
- 外部函数：`trace_ocfs2_query_inode_wipe_end`
- 外部函数：`trace_ocfs2_query_inode_wipe_succ`
- 外部函数：`truncate_inode_pages_final`
- 外部函数：`unlikely`
- 外部函数：`up_write`
- 外部函数：`wake_up`
- 外部函数：`write_inode_now`
- 大写宏：`ABBA`
- 大写宏：`EAGAIN`
- 大写宏：`EDEADLK`
- 大写宏：`EEXIST`
- 大写宏：`ENOENT`
- 大写宏：`ERESTARTSYS`
- 大写宏：`HAS_REFCOUNT_FL`
- 大写宏：`INODE_ALLOC_SYSTEM_INODE`
- 大写宏：`INODE_CACHE`
- 大写宏：`IS_ERR`
- 大写宏：`LVB`
- 大写宏：`ML_ERROR`
- 大写宏：`NULL`
- 大写宏：`OCFS2`
- 大写宏：`OCFS2_CACHE_FL_INLINE`
- 大写宏：`OCFS2_DELETE_INODE_CREDITS`
- 大写宏：`OCFS2_DIO_ORPHANED_FL`
- 大写宏：`OCFS2_HAS_REFCOUNT_FL`
- 大写宏：`OCFS2_I`
- 大写宏：`OCFS2_INODE_DELETED`
- 大写宏：`OCFS2_INODE_MAYBE_ORPHANED`
- 大写宏：`OCFS2_INODE_SKIP_ORPHAN_DIR`
- 大写宏：`OCFS2_INODE_SYSTEM_FILE`
- 大写宏：`OCFS2_INODE_UPDATE_CREDITS`
- 大写宏：`OCFS2_JOURNAL_ACCESS_WRITE`
- 大写宏：`OCFS2_ORPHANED_FL`
- 大写宏：`OCFS2_SB`
- 大写宏：`OCFS2_VALID_FL`
- 大写宏：`ORPHANED_FL`
- 大写宏：`ORPHAN_DIR_SYSTEM_INODE`
- 大写宏：`PTR_ERR`
- 大写宏：`S_ISDIR`
- 外部类型：`But`
- 外部类型：`Clear`
- 外部类型：`Disk`
- 外部类型：`Do`
- 外部类型：`Error`
- 外部类型：`Even`
- 外部类型：`Every`
- 外部类型：`Free`
- 外部类型：`If`
- 外部类型：`Inode`
- 外部类型：`Inodes`
- 外部类型：`It`
- 外部类型：`Lock`
- 外部类型：`Mark`
- 外部类型：`Now`
- 外部类型：`Of`
- 外部类型：`Query`
- 外部类型：`Recheck`
- 外部类型：`Remove`
- 外部类型：`Skip`
- 外部类型：`Skipping`
- 外部类型：`Someone`
- 外部类型：`Synchronize`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Though`
- 外部类型：`Thus`
- 外部类型：`To`
- 外部类型：`Trylocks`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`Which`
- 外部类型：`While`
- 外部类型：`Write`
- 外部类型：`buffer_head`
- 外部类型：`handle_t`
- 外部类型：`inode`
- 外部类型：`ocfs2_dinode`
- 外部类型：`ocfs2_inode_info`
- 外部类型：`ocfs2_super`
- 外部类型：`sigset_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-1f5da7f2b6` → 本草稿移入 `cases/defect/auto-linux-1f5da7f2b6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
