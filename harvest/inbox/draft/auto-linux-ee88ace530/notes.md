# auto-linux-ee88ace530

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#58aeff92eebcad28da1f01e6d16b04794f7ea4f2](https://github.com/torvalds/linux/commit/58aeff92eebcad28da1f01e6d16b04794f7ea4f2) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 154 |
| 编译错误数（gcc syntax-only） | 209（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #58aeff92eebcad28da1f01e6d16b04794f7ea4f2 (https://github.com/torvalds/linux/commit/58aeff92eebcad28da1f01e6d16b04794f7ea4f2)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1377；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 58aeff92eebcad28da1f01e6d16b04794f7ea4f2 Merge tag 'vfs-7.4-rc1.shared.super' of git://git.kernel.org/pub/scm/linux/kernel/git/vfs/vfs :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1374,7 +1374,17 @@ static int test_single_super(struct super_block *s, struct fs_context *fc)
 	return 1;
 }
 
-static int vfs_get_super(struct fs_context *fc,
+/**
+ * get_tree_super - Get a superblock, optionally sharing an existing one
+ * @fc: The filesystem context holding the parameters
+ * @test: Comparison function to find a matching existing superblock, or NULL
+ * @fill_super: Helper to initialise a new superblock
+ *
+ * If @test is non-NULL and matches an existing superblock, that superblock is
+ * reused; otherwise a new anonymous superblock is created and initialised with
+ * @fill_super.  Passing NULL for @test always creates a new superblock.
+ */
+int get_tree_super(struct fs_context *fc,
 		int (*test)(struct super_block *, struct fs_context *),
 		int (*fill_super)(struct super_block *sb,
 				  struct fs_context *fc))
@@ -1401,20 +1411,21 @@ static int vfs_get_super(struct fs_context *fc,
 	deactivate_locked_super(sb);
 	return err;
 }
+EXPORT_SYMBOL_GPL(get_tree_super);
 
 int get_tree_nodev(struct fs_context *fc,
 		  int (*fill_super)(struct super_block *sb,
 				    struct fs_context *fc))
 {
-	return vfs_get_super(fc, NULL, fill_super);
+	return get_tree_super(fc, NULL, fill_super);
 }
 EXPORT_SYMBOL(get_tree_nodev);
 
 int get_tree_single(struct fs_context *fc,
 		  int (*fill_super)(struct super_block *sb,
 				    struct fs_context *fc))
 {
-	return vfs_get_super(fc, test_single_super, fill_super);
+	return get_tree_super(fc, test_single_super, fill_super);
 }
 EXPORT_SYMBOL(get_tree_single);
 
@@ -1424,7 +1435,7 @@ int get_tree_keyed(struct fs_context *fc,
 		void *key)
 {
 	fc->s_fs_info = key;
-	return vfs_get_super(fc, test_keyed_super, fill_super);
+	return get_tree_super(fc, test_keyed_super, fill_super);
 }
 EXPORT_SYMBOL(get_tree_keyed);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`___wait_var_event`
- 外部函数：`__percpu_init_rwsem`
- 外部函数：`__super_lock`
- 外部函数：`atomic_dec_and_test`
- 外部函数：`atomic_inc_not_zero`
- 外部函数：`atomic_set`
- 外部函数：`call_rcu`
- 外部函数：`clear_inode`
- 外部函数：`container_of`
- 外部函数：`dget`
- 外部函数：`down_read_trylock`
- 外部函数：`down_write_nested`
- 外部函数：`errorfc`
- 外部函数：`fill_super`
- 外部函数：`free_cached_objects`
- 外部函数：`fserror_mount`
- 外部函数：`fsnotify_sb_free`
- 外部函数：`get_filesystem`
- 外部函数：`get_user_ns`
- 外部函数：`hlist_add_head`
- 外部函数：`hlist_del_init`
- 外部函数：`hweight32`
- 外部函数：`ida_alloc_range`
- 外部函数：`ida_free`
- 外部函数：`init_rwsem`
- 外部函数：`int`
- 外部函数：`kfree`
- 外部函数：`kfree_rcu`
- 外部函数：`kill_sb`
- 外部函数：`kzalloc_obj`
- 外部函数：`list_add_tail`
- 外部函数：`list_del_init`
- 外部函数：`list_lru_destroy`
- 外部函数：`list_lru_init_memcg`
- 外部函数：`list_lru_shrink_count`
- 外部函数：`lockdep_assert_held`
- 外部函数：`lockdep_assert_not_held`
- 外部函数：`lockdep_set_class`
- 外部函数：`mount_fs`
- 外部函数：`mult_frac`
- 外部函数：`mutex_init`
- 外部函数：`nr_cached_objects`
- 外部函数：`one`
- 外部函数：`percpu_free_rwsem`
- 外部函数：`prepare_to_wait_event`
- 外部函数：`prune_dcache_sb`
- 外部函数：`prune_icache_sb`
- 外部函数：`put_filesystem`
- 外部函数：`put_user_ns`
- 外部函数：`refcount_dec_and_test`
- 外部函数：`refcount_inc`
- 外部函数：`refcount_set`
- 外部函数：`rhltable_insert`
- 外部函数：`rhltable_remove`
- 外部函数：`schedule_work`
- 外部函数：`security_sb_alloc`
- 外部函数：`security_sb_free`
- 外部函数：`set`
- 外部函数：`shrinker_alloc`
- 外部函数：`shrinker_free`
- 外部函数：`shrinker_register`
- 外部函数：`smp_load_acquire`
- 外部函数：`smp_mb`
- 外部函数：`smp_rmb`
- 外部函数：`smp_store_release`
- 外部函数：`spin_lock`
- 外部函数：`spin_lock_init`
- 外部函数：`spin_unlock`
- 外部函数：`strscpy`
- 外部函数：`test`
- 外部函数：`up_read`
- 外部函数：`up_write`
- 外部函数：`vfs_pressure_ratio`
- 外部函数：`wait_var_event`
- 外部函数：`waitqueue_active`
- 外部函数：`wake_up_var`
- 外部函数：`warnfc`
- 大写宏：`EBUSY`
- 大写宏：`EMFILE`
- 大写宏：`ENOMEM`
- 大写宏：`ENOSPC`
- 大写宏：`EPERM`
- 大写宏：`ERR_PTR`
- 大写宏：`EXPORT_SYMBOL`
- 大写宏：`FSID`
- 大写宏：`FS_USERNS_DELEGATABLE`
- 大写宏：`FS_USERNS_MOUNT`
- 大写宏：`GFP_ATOMIC`
- 大写宏：`INIT_HLIST_BL_HEAD`
- 大写宏：`INIT_HLIST_NODE`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`INIT_WORK`
- 大写宏：`IS_ERR`
- 大写宏：`MAX_NON_LFS`
- 大写宏：`MINOR`
- 大写宏：`MINORBITS`
- 大写宏：`MIN_WRITEBACK_PAGES`
- 大写宏：`MKDEV`
- 大写宏：`NULL`
- 大写宏：`PTR_ERR`
- 大写宏：`SB_ACTIVE`
- 大写宏：`SB_BORN`
- 大写宏：`SB_DEAD`
- 大写宏：`SB_DYING`
- 大写宏：`SB_FREEZE_LEVELS`
- 大写宏：`SB_I_NODEV`
- 大写宏：`SHRINKER_MEMCG_AWARE`
- 大写宏：`SHRINKER_NUMA_AWARE`
- 大写宏：`SHRINK_EMPTY`
- 大写宏：`SHRINK_STOP`
- 大写宏：`SINGLE_DEPTH_NESTING`
- 大写宏：`TIME64_MAX`
- 大写宏：`TIME64_MIN`
- 大写宏：`VFS`
- 大写宏：`VFS_WARN_ON_ONCE`
- 大写宏：`WARN_ON`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Always`
- 外部类型：`Annotate`
- 外部类型：`Counts`
- 外部类型：`Deadlock`
- 外部类型：`Don`
- 外部类型：`Drop`
- 外部类型：`Ensure`
- 外部类型：`Everyone`
- 外部类型：`Has`
- 外部类型：`However`
- 外部类型：`In`
- 外部类型：`It`
- 外部类型：`Let`
- 外部类型：`Make`
- 外部类型：`Many`
- 外部类型：`Mounting`
- 外部类型：`Never`
- 外部类型：`No`
- 外部类型：`Only`
- 外部类型：`Pairs`
- 外部类型：`Since`
- 外部类型：`Tell`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Unlink`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`dev_t`
- 外部类型：`file_system_type`
- 外部类型：`fs_context`
- 外部类型：`rcu_head`
- 外部类型：`refcount_t`
- 外部类型：`rhlist_head`
- 外部类型：`shrink_control`
- 外部类型：`shrinker`
- 外部类型：`super_block`
- 外部类型：`super_operations`
- 外部类型：`user_namespace`
- 外部类型：`work_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-ee88ace530` → 本草稿移入 `cases/defect/auto-linux-ee88ace530/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
