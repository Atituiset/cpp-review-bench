# auto-linux-a1d90e0d8a

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
| 外部依赖数（dep_count） | 36 |
| 编译错误数（gcc syntax-only） | 99（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd (https://github.com/torvalds/linux/commit/4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 278；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd Merge tag 'mm-nonmm-hotfixes-stable-2026-11-18-00-06' of git://git.kernel.org/pub/scm/linux/kernel/git/akpm/mm :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -162,7 +162,13 @@ static void user_dlm_unblock_lock(struct work_struct *work);
 
 static void __user_dlm_queue_lockres(struct user_lock_res *lockres)
 {
+	struct inode *inode = user_dlm_inode_from_user_lockres(lockres);
+
 	if (!(lockres->l_flags & USER_LOCK_QUEUED)) {
+		/* Pin the superblock through the worker's iput(). */
+		if (!atomic_inc_not_zero(&inode->i_sb->s_active))
+			return;
+
 		user_dlm_grab_inode_ref(lockres);
 
 		INIT_WORK(&lockres->l_work, user_dlm_unblock_lock);
@@ -275,9 +281,11 @@ static struct ocfs2_locking_protocol user_dlm_lproto = {
 
 static inline void user_dlm_drop_inode_ref(struct user_lock_res *lockres)
 {
-	struct inode *inode;
-	inode = user_dlm_inode_from_user_lockres(lockres);
+	struct inode *inode = user_dlm_inode_from_user_lockres(lockres);
+	struct super_block *sb = inode->i_sb;
+
 	iput(inode);
+	deactivate_super(sb);
 }
 
 static void user_dlm_unblock_lock(struct work_struct *work)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`container_of`
- 外部函数：`igrab`
- 外部函数：`iput`
- 外部函数：`mlog`
- 外部函数：`mlog_bug_on_msg`
- 外部函数：`ocfs2_dlm_lock`
- 外部函数：`ocfs2_dlm_unlock`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`user_highest_compat_lock_level`
- 外部函数：`user_log_dlm_error`
- 大写宏：`BUG`
- 大写宏：`DLM_LKF_CANCEL`
- 大写宏：`DLM_LKF_CONVERT`
- 大写宏：`DLM_LKF_VALBLK`
- 大写宏：`DLM_LOCK_EX`
- 大写宏：`DLM_LOCK_NL`
- 大写宏：`DLM_LOCK_PR`
- 大写宏：`INIT_WORK`
- 大写宏：`ML_BASTS`
- 大写宏：`ML_ERROR`
- 大写宏：`USER_LOCK_BLOCKED`
- 大写宏：`USER_LOCK_BUSY`
- 大写宏：`USER_LOCK_IN_CANCEL`
- 大写宏：`USER_LOCK_IN_TEARDOWN`
- 大写宏：`USER_LOCK_QUEUED`
- 外部类型：`Dlm`
- 外部类型：`Holders`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`Lockres`
- 外部类型：`dlmfs_inode_private`
- 外部类型：`inode`
- 外部类型：`ocfs2_cluster_connection`
- 外部类型：`user_lock_res`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-a1d90e0d8a` → 本草稿移入 `cases/defect/auto-linux-a1d90e0d8a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
