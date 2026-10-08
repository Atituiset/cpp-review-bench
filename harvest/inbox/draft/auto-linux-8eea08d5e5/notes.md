# auto-linux-8eea08d5e5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7b63ef2d55f24519e7e9e5f4d15dbea03f126e40](https://github.com/torvalds/linux/commit/7b63ef2d55f24519e7e9e5f4d15dbea03f126e40) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 28 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7b63ef2d55f24519e7e9e5f4d15dbea03f126e40 (https://github.com/torvalds/linux/commit/7b63ef2d55f24519e7e9e5f4d15dbea03f126e40)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 5009；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7b63ef2d55f24519e7e9e5f4d15dbea03f126e40 Merge tag 'for-7.3-rc6-tag' of git://git.kernel.org/pub/scm/linux/kernel/git/kdave/linux :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4601,7 +4601,7 @@ static void btrfs_uring_read_finished(struct io_tw_req tw_req, io_tw_token_t tw)
 	size_t page_offset;
 	ssize_t ret;
 
-	/* The inode lock has already been acquired in btrfs_uring_read_extent.  */
+	/* The inode lock has already been acquired in btrfs_encoded_read(). */
 	btrfs_lockdep_inode_acquire(inode, i_rwsem);
 
 	if (priv->err) {
@@ -4667,7 +4667,6 @@ static int btrfs_uring_read_extent(struct kiocb *iocb, struct iov_iter *iter,
 				   struct iovec *iov, struct io_uring_cmd *cmd)
 {
 	struct btrfs_inode *inode = BTRFS_I(file_inode(iocb->ki_filp));
-	struct extent_io_tree *io_tree = &inode->io_tree;
 	struct page **pages = NULL;
 	struct btrfs_uring_priv *priv = NULL;
 	unsigned long nr_pages;
@@ -4723,8 +4722,6 @@ static int btrfs_uring_read_extent(struct kiocb *iocb, struct iov_iter *iter,
 	return -EIOCBQUEUED;
 
 out_fail:
-	btrfs_unlock_extent(io_tree, start, lockend, &cached_state);
-	btrfs_inode_unlock(inode, BTRFS_ILOCK_SHARED);
 	kfree(priv);
 	for (int i = 0; i < nr_pages; i++) {
 		if (pages[i])
@@ -4837,7 +4834,7 @@ static int btrfs_uring_encoded_read(struct io_uring_cmd *cmd, unsigned int issue
 	ret = btrfs_encoded_read(&kiocb, &data->iter, &data->args, &cached_state,
 				 &disk_bytenr, &disk_io_size);
 	if (ret == -EAGAIN)
-		goto out_acct;
+		goto out_free;
 	if (ret < 0 && ret != -EIOCBQUEUED)
 		goto out_free;
 
@@ -4865,8 +4862,10 @@ static int btrfs_uring_encoded_read(struct io_uring_cmd *cmd, unsigned int issue
 					      cached_state, disk_bytenr, disk_io_size,
 					      count, data->args.compression,
 					      data->iov, cmd);
-
-		goto out_acct;
+		if (ret == -EIOCBQUEUED)
+			goto out_acct;
+		btrfs_unlock_extent(io_tree, start, lockend, &cached_state);
+		btrfs_inode_unlock(inode, BTRFS_ILOCK_SHARED);
 	}
 
 out_free:
@@ -4877,8 +4876,10 @@ static int btrfs_uring_encoded_read(struct io_uring_cmd *cmd, unsigned int issue
 		add_rchar(current, ret);
 	inc_syscr(current);
 
-	if (ret != -EIOCBQUEUED && ret != -EAGAIN)
+	if (ret != -EIOCBQUEUED) {
 		kfree(data);
+		bc->data = NULL;
+	}
 
 	return ret;
 }
@@ -4907,6 +4908,11 @@ static int btrfs_uring_encoded_write(struct io_uring_cmd *cmd, unsigned int issu
 		goto out_acct;
 	}
 
+	if (issue_flags & IO_URING_F_NONBLOCK) {
+		ret = -EAGAIN;
+		goto out_acct;
+	}
+
 	if (!data) {
 		data = kzalloc_obj(*data, GFP_NOFS);
 		if (!data) {
@@ -4975,11 +4981,6 @@ static int btrfs_uring_encoded_write(struct io_uring_cmd *cmd, unsigned int issu
 		}
 	}
 
-	if (issue_flags & IO_URING_F_NONBLOCK) {
-		ret = -EAGAIN;
-		goto out_acct;
-	}
-
 	pos = data->args.offset;
 	ret = rw_verify_area(WRITE, file, &pos, data->args.len);
 	if (ret < 0)
@@ -5005,8 +5006,8 @@ static int btrfs_uring_encoded_write(struct io_uring_cmd *cmd, unsigned int issu
 		add_wchar(current, ret);
 	inc_syscw(current);
 
-	if (ret != -EAGAIN)
-		kfree(data);
+	kfree(data);
+	bc->data = NULL;
 	return ret;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`add_rchar`
- 外部函数：`add_wchar`
- 外部函数：`btrfs_encoded_read`
- 外部函数：`btrfs_inode_unlock`
- 外部函数：`btrfs_lockdep_inode_acquire`
- 外部函数：`btrfs_unlock_extent`
- 外部函数：`file_inode`
- 外部函数：`inc_syscr`
- 外部函数：`inc_syscw`
- 外部函数：`kfree`
- 外部函数：`kzalloc_obj`
- 外部函数：`rw_verify_area`
- 大写宏：`BTRFS_I`
- 大写宏：`BTRFS_ILOCK_SHARED`
- 大写宏：`EAGAIN`
- 大写宏：`EIOCBQUEUED`
- 大写宏：`GFP_NOFS`
- 大写宏：`IO_URING_F_NONBLOCK`
- 大写宏：`NULL`
- 大写宏：`WRITE`
- 外部类型：`The`
- 外部类型：`btrfs_inode`
- 外部类型：`extent_io_tree`
- 外部类型：`extent_state`
- 外部类型：`io_uring_cmd`
- 外部类型：`iov_iter`
- 外部类型：`iovec`
- 外部类型：`kiocb`
- 外部类型：`page`
- 外部类型：`size_t`
- 外部类型：`ssize_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-8eea08d5e5` → 本草稿移入 `cases/defect/auto-linux-8eea08d5e5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
