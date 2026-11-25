# auto-linux-7794a2e1fe

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
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 46（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4dce97c6c265544342e716e62fd24bbb232590c5 (https://github.com/torvalds/linux/commit/4dce97c6c265544342e716e62fd24bbb232590c5)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4dce97c6c265544342e716e62fd24bbb232590c5 Merge tag 'ext4_for_linus-7.4-rc5' of git://git.kernel.org/pub/scm/linux/kernel/git/tytso/ext4 :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -15,6 +15,7 @@ unsigned int ext4_count_free(char *bitmap, unsigned int numchars)
 {
 	return numchars * BITS_PER_BYTE - memweight(bitmap, numchars);
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_count_free);
 
 int ext4_inode_bitmap_csum_verify(struct super_block *sb,
 				  struct ext4_group_desc *gdp,
@@ -39,6 +40,7 @@ int ext4_inode_bitmap_csum_verify(struct super_block *sb,
 
 	return provided == calculated;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_inode_bitmap_csum_verify);
 
 void ext4_inode_bitmap_csum_set(struct super_block *sb,
 				struct ext4_group_desc *gdp,
@@ -57,6 +59,7 @@ void ext4_inode_bitmap_csum_set(struct super_block *sb,
 	if (sbi->s_desc_size >= EXT4_BG_INODE_BITMAP_CSUM_HI_END)
 		gdp->bg_inode_bitmap_csum_hi = cpu_to_le16(csum >> 16);
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_inode_bitmap_csum_set);
 
 int ext4_block_bitmap_csum_verify(struct super_block *sb,
 				  struct ext4_group_desc *gdp,
@@ -80,6 +83,7 @@ int ext4_block_bitmap_csum_verify(struct super_block *sb,
 
 	return provided == calculated;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_block_bitmap_csum_verify);
 
 void ext4_block_bitmap_csum_set(struct super_block *sb,
 				struct ext4_group_desc *gdp,
@@ -97,3 +101,4 @@ void ext4_block_bitmap_csum_set(struct super_block *sb,
 	if (sbi->s_desc_size >= EXT4_BG_BLOCK_BITMAP_CSUM_HI_END)
 		gdp->bg_block_bitmap_csum_hi = cpu_to_le16(csum >> 16);
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_block_bitmap_csum_set);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cpu_to_le16`
- 外部函数：`ext4_chksum`
- 外部函数：`ext4_has_feature_metadata_csum`
- 外部函数：`le16_to_cpu`
- 外部函数：`memweight`
- 大写宏：`BITS_PER_BYTE`
- 大写宏：`EXT4_BG_BLOCK_BITMAP_CSUM_HI_END`
- 大写宏：`EXT4_BG_INODE_BITMAP_CSUM_HI_END`
- 大写宏：`EXT4_CLUSTERS_PER_GROUP`
- 大写宏：`EXT4_INODES_PER_GROUP`
- 大写宏：`EXT4_SB`
- 外部类型：`buffer_head`
- 外部类型：`ext4_group_desc`
- 外部类型：`ext4_sb_info`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-7794a2e1fe` → 本草稿移入 `cases/defect/auto-linux-7794a2e1fe/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
