# auto-linux-46f37e7d2c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#4dce97c6c265544342e716e62fd24bbb232590c5](https://github.com/torvalds/linux/commit/4dce97c6c265544342e716e62fd24bbb232590c5) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 54 |
| 编译错误数（gcc syntax-only） | 42（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4dce97c6c265544342e716e62fd24bbb232590c5 (https://github.com/torvalds/linux/commit/4dce97c6c265544342e716e62fd24bbb232590c5)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 492；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4dce97c6c265544342e716e62fd24bbb232590c5 Merge tag 'ext4_for_linus-7.4-rc5' of git://git.kernel.org/pub/scm/linux/kernel/git/tytso/ext4 :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -243,6 +243,7 @@ unsigned ext4_free_clusters_after_init(struct super_block *sb,
 	return num_clusters_in_group(sb, block_group) -
 		ext4_num_overhead_clusters(sb, block_group, gdp);
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_free_clusters_after_init);
 
 /*
  * The free blocks are managed by bitmaps.  A file system contains several
@@ -489,7 +490,7 @@ ext4_read_block_bitmap_nowait(struct super_block *sb, ext4_group_t block_group,
 					EXT4_GROUP_INFO_BBITMAP_CORRUPT);
 		return ERR_PTR(-EFSCORRUPTED);
 	}
-	bh = sb_getblk(sb, bitmap_blk);
+	bh = ext4_sb_getblk(sb, bitmap_blk);
 	if (unlikely(!bh)) {
 		ext4_warning(sb, "Cannot get buffer for block bitmap - "
 			     "block_group = %u, block_bitmap = %llu",
@@ -611,6 +612,7 @@ ext4_read_block_bitmap(struct super_block *sb, ext4_group_t block_group)
 	}
 	return bh;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_read_block_bitmap);
 
 /**
  * ext4_has_free_clusters()
@@ -678,6 +680,7 @@ int ext4_claim_free_clusters(struct ext4_sb_info *sbi,
 	} else
 		return -ENOSPC;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_claim_free_clusters);
 
 /**
  * ext4_should_retry_alloc() - check if a block allocation should be retried
@@ -724,6 +727,7 @@ int ext4_should_retry_alloc(struct super_block *sb, int *retries)
 	(void) jbd2_journal_force_commit_nested(sbi->s_journal);
 	return 1;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_should_retry_alloc);
 
 /*
  * ext4_new_meta_blocks() -- allocate block for meta data (indexing) blocks
@@ -764,6 +768,7 @@ ext4_fsblk_t ext4_new_meta_blocks(handle_t *handle, struct inode *inode,
 	}
 	return ret;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_new_meta_blocks);
 
 /**
  * ext4_count_free_clusters() -- count filesystem free clusters
@@ -877,6 +882,7 @@ int ext4_bg_has_super(struct super_block *sb, ext4_group_t group)
 
 	return 0;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_bg_has_super);
 
 static unsigned long ext4_bg_num_gdb_meta(struct super_block *sb,
 					ext4_group_t group)
@@ -923,6 +929,7 @@ unsigned long ext4_bg_num_gdb(struct super_block *sb, ext4_group_t group)
 	return ext4_bg_num_gdb_meta(sb,group);
 
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_bg_num_gdb);
 
 /*
  * This function returns the number of file system metadata blocks at
@@ -949,6 +956,7 @@ unsigned int ext4_num_base_meta_blocks(struct super_block *sb,
 	}
 	return num;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_num_base_meta_blocks);
 
 static unsigned int ext4_num_base_meta_clusters(struct super_block *sb,
 						ext4_group_t block_group)
@@ -1004,4 +1012,5 @@ ext4_fsblk_t ext4_inode_to_goal_block(struct inode *inode)
 			((last_block - bg_start) / 16);
 	return bg_start + colour;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_inode_to_goal_block);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`data`
- 外部函数：`do_div`
- 外部函数：`ext4_block_bitmap`
- 外部函数：`ext4_blocks_count`
- 外部函数：`ext4_get_groups_count`
- 外部函数：`ext4_group_first_block_no`
- 外部函数：`ext4_has_feature_sparse_super`
- 外部函数：`ext4_has_feature_sparse_super2`
- 外部函数：`ext4_has_free_clusters`
- 外部函数：`ext4_inode_bitmap`
- 外部函数：`ext4_inode_table`
- 外部函数：`ext4_new_meta_blocks`
- 外部函数：`ext4_num_base_meta_clusters`
- 外部函数：`ext4_should_retry_alloc`
- 外部函数：`ext4_warning`
- 外部函数：`jbd2_journal_force_commit_nested`
- 外部函数：`le32_to_cpu`
- 外部函数：`max`
- 外部函数：`min`
- 外部函数：`sb_getblk`
- 外部函数：`test_opt2`
- 外部函数：`test_root`
- 外部函数：`unlikely`
- 大写宏：`EFSCORRUPTED`
- 大写宏：`ENOSPC`
- 大写宏：`ERR_PTR`
- 大写宏：`EXT4_B2C`
- 大写宏：`EXT4_BLOCKS_PER_GROUP`
- 大写宏：`EXT4_BLOCK_SIZE_BITS`
- 大写宏：`EXT4_CLUSTER_BITS`
- 大写宏：`EXT4_GROUP_INFO_BBITMAP_CORRUPT`
- 大写宏：`EXT4_NUM_B2C`
- 大写宏：`EXT4_SB`
- 大写宏：`NULL`
- 大写宏：`STD_GROUP_SIZE`
- 外部类型：`Account`
- 外部类型：`Additional`
- 外部类型：`Cannot`
- 外部类型：`Even`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`Normally`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`ext4_fsblk_t`
- 外部类型：`ext4_group_desc`
- 外部类型：`ext4_group_t`
- 外部类型：`ext4_grpblk_t`
- 外部类型：`ext4_sb_info`
- 外部类型：`ext4_super_block`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-46f37e7d2c` → 本草稿移入 `cases/defect/auto-linux-46f37e7d2c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
