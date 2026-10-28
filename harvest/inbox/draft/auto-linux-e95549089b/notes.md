# auto-linux-e95549089b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7d1f84f1b94e79cb95d974ab3ec54cb3a4d64f5b](https://github.com/torvalds/linux/commit/7d1f84f1b94e79cb95d974ab3ec54cb3a4d64f5b) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 26 |
| 编译错误数（gcc syntax-only） | 46（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7d1f84f1b94e79cb95d974ab3ec54cb3a4d64f5b (https://github.com/torvalds/linux/commit/7d1f84f1b94e79cb95d974ab3ec54cb3a4d64f5b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 609；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7d1f84f1b94e79cb95d974ab3ec54cb3a4d64f5b Merge tag 'mm-nonmm-stable-2026-10-25-14-57' of git://git.kernel.org/pub/scm/linux/kernel/git/akpm/mm :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -413,7 +413,7 @@ static int fat_mirror_bhs(struct super_block *sb, struct buffer_head **bhs,
 }
 
 int fat_ent_write(struct inode *inode, struct fat_entry *fatent,
-		  int new, int wait)
+		  int new, int old, int wait)
 {
 	struct super_block *sb = inode->i_sb;
 	const struct fatent_operations *ops = MSDOS_SB(sb)->fatent_ops;
@@ -422,10 +422,25 @@ int fat_ent_write(struct inode *inode, struct fat_entry *fatent,
 	ops->ent_put(fatent, new);
 	if (wait) {
 		err = fat_sync_bhs(fatent->bhs, fatent->nr_bhs);
-		if (err)
+		if (err) {
+			/*
+			 * bhs are not uptodate after I/O error. So we
+			 * can't simply re-dirty to revert. And it
+			 * would not have value to write again on I/O
+			 * error.
+			 */
 			return err;
+		}
 	}
-	return fat_mirror_bhs(sb, fatent->bhs, fatent->nr_bhs);
+
+	err = fat_mirror_bhs(sb, fatent->bhs, fatent->nr_bhs);
+	if (err) {
+		/* Try to revert if got the error on mirror FAT */
+		ops->ent_put(fatent, old);
+		if (wait)
+			fat_sync_bhs(fatent->bhs, fatent->nr_bhs);
+	}
+	return err;
 }
 
 static inline int fat_ent_next(struct msdos_sb_info *sbi,
@@ -468,7 +483,28 @@ static void fat_collect_bhs(struct buffer_head **bhs, int *nr_bhs,
 	}
 }
 
-int fat_alloc_clusters(struct inode *inode, int *cluster, int nr_cluster)
+static int fat_flush_bhs(struct super_block *sb, struct buffer_head **bhs,
+			 int *nr_bhs, bool sync)
+{
+	int err, i;
+
+	if (sync) {
+		err = fat_sync_bhs(bhs, *nr_bhs);
+		if (err)
+			return err;
+	}
+	err = fat_mirror_bhs(sb, bhs, *nr_bhs);
+	if (err)
+		return err;
+	for (i = 0; i < *nr_bhs; i++)
+		brelse(bhs[i]);
+	*nr_bhs = 0;
+
+	return 0;
+}
+
+static int __fat_alloc_clusters(struct inode *inode, int *cluster, int nr_cluster,
+				bool return_all)
 {
 	struct super_block *sb = inode->i_sb;
 	struct msdos_sb_info *sbi = MSDOS_SB(sb);
@@ -477,8 +513,6 @@ int fat_alloc_clusters(struct inode *inode, int *cluster, int nr_cluster)
 	struct buffer_head *bhs[MAX_BUF_PER_PAGE];
 	int i, count, err, nr_bhs, idx_clus;
 
-	BUG_ON(nr_cluster > (MAX_BUF_PER_PAGE / 2));	/* fixed limit */
-
 	lock_fat(sbi);
 	if (sbi->free_clusters != -1 && sbi->free_clus_valid &&
 	    sbi->free_clusters < nr_cluster) {
@@ -509,14 +543,24 @@ int fat_alloc_clusters(struct inode *inode, int *cluster, int nr_cluster)
 				if (prev_ent.nr_bhs)
 					ops->ent_put(&prev_ent, entry);
 
-				fat_collect_bhs(bhs, &nr_bhs, &fatent);
-
 				sbi->prev_free = entry;
 				if (sbi->free_clusters != -1)
 					sbi->free_clusters--;
 
-				cluster[idx_clus] = entry;
+				if (return_all)
+					cluster[idx_clus] = entry;
+				else if (!idx_clus)
+					*cluster = entry;
 				idx_clus++;
+
+				if (nr_bhs + fatent.nr_bhs > MAX_BUF_PER_PAGE) {
+					err = fat_flush_bhs(sb, bhs, &nr_bhs,
+							    inode_needs_sync(inode));
+					if (err)
+						goto out;
+				}
+				fat_collect_bhs(bhs, &nr_bhs, &fatent);
+
 				if (idx_clus == nr_cluster)
 					goto out;
 
@@ -556,6 +600,16 @@ int fat_alloc_clusters(struct inode *inode, int *cluster, int nr_cluster)
 	return err;
 }
 
+int fat_alloc_clusters(struct inode *inode, int *cluster, int nr_cluster)
+{
+	return __fat_alloc_clusters(inode, cluster, nr_cluster, true);
+}
+
+int fat_alloc_chain(struct inode *inode, int *cluster, int nr_cluster)
+{
+	return __fat_alloc_clusters(inode, cluster, nr_cluster, false);
+}
+
 int fat_free_clusters(struct inode *inode, int cluster)
 {
 	struct super_block *sb = inode->i_sb;
@@ -606,17 +660,10 @@ int fat_free_clusters(struct inode *inode, int cluster)
 		}
 
 		if (nr_bhs + fatent.nr_bhs > MAX_BUF_PER_PAGE) {
-			if (sb->s_flags & SB_SYNCHRONOUS) {
-				err = fat_sync_bhs(bhs, nr_bhs);
-				if (err)
-					goto error;
-			}
-			err = fat_mirror_bhs(sb, bhs, nr_bhs);
+			err = fat_flush_bhs(sb, bhs, &nr_bhs,
+					    sb->s_flags & SB_SYNCHRONOUS);
 			if (err)
 				goto error;
-			for (i = 0; i < nr_bhs; i++)
-				brelse(bhs[i]);
-			nr_bhs = 0;
 		}
 		fat_collect_bhs(bhs, &nr_bhs, &fatent);
 	} while (cluster != FAT_ENT_EOF);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`brelse`
- 外部函数：`ent_next`
- 外部函数：`ent_put`
- 外部函数：`fat_sync_bhs`
- 外部函数：`get_bh`
- 外部函数：`int`
- 外部函数：`lock_buffer`
- 外部函数：`memcpy`
- 外部函数：`mmb_mark_buffer_dirty`
- 外部函数：`mutex_lock`
- 外部函数：`sb_getblk`
- 外部函数：`set_buffer_uptodate`
- 外部函数：`sync_dirty_buffer`
- 外部函数：`unlock_buffer`
- 外部函数：`void`
- 大写宏：`BUG_ON`
- 大写宏：`ENOMEM`
- 大写宏：`FAT_ENT_EOF`
- 大写宏：`MAX_BUF_PER_PAGE`
- 大写宏：`MSDOS_I`
- 大写宏：`MSDOS_SB`
- 大写宏：`SB_SYNCHRONOUS`
- 外部类型：`Avoid`
- 外部类型：`buffer_head`
- 外部类型：`fat_entry`
- 外部类型：`inode`
- 外部类型：`msdos_sb_info`
- 外部类型：`sector_t`
- 外部类型：`super_block`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e95549089b` → 本草稿移入 `cases/defect/auto-linux-e95549089b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
