# auto-linux-8f73510a8e

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
| 外部依赖数（dep_count） | 28 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4dce97c6c265544342e716e62fd24bbb232590c5 (https://github.com/torvalds/linux/commit/4dce97c6c265544342e716e62fd24bbb232590c5)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 80；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4dce97c6c265544342e716e62fd24bbb232590c5 Merge tag 'ext4_for_linus-7.4-rc5' of git://git.kernel.org/pub/scm/linux/kernel/git/tytso/ext4 :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -17,6 +17,7 @@
 #include <linux/swap.h>
 #include <linux/pagemap.h>
 #include <linux/blkdev.h>
+#include <linux/export.h>
 #include <linux/slab.h>
 #include "ext4.h"
 
@@ -74,10 +75,13 @@ static int add_system_zone(struct ext4_system_blocks *system_blks,
 	struct rb_node **n = &system_blks->root.rb_node, *node;
 	struct rb_node *parent = NULL, *new_node;
 
+	if (!count || (start_blk + count < start_blk))
+		return -EFSCORRUPTED;
+
 	while (*n) {
 		parent = *n;
 		entry = rb_entry(parent, struct ext4_system_zone, node);
-		if (start_blk < entry->start_blk)
+		if (start_blk + count <= entry->start_blk)
 			n = &(*n)->rb_left;
 		else if (start_blk >= (entry->start_blk + entry->count))
 			n = &(*n)->rb_right;
@@ -269,6 +273,7 @@ int ext4_setup_system_zone(struct super_block *sb)
 	kfree(system_blks);
 	return ret;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_setup_system_zone);
 
 /*
  * Called when the filesystem is unmounted or when remounting it with
@@ -291,6 +296,7 @@ void ext4_release_system_zone(struct super_block *sb)
 	if (system_blks)
 		call_rcu(&system_blks->rcu, ext4_destroy_system_zone);
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_release_system_zone);
 
 int ext4_sb_block_valid(struct super_block *sb, struct inode *inode,
 				ext4_fsblk_t start_blk, unsigned int count)
@@ -334,6 +340,7 @@ int ext4_sb_block_valid(struct super_block *sb, struct inode *inode,
 	rcu_read_unlock();
 	return ret;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_sb_block_valid);
 
 /*
  * Returns 1 if the passed-in block region (start_blk,
@@ -345,6 +352,7 @@ int ext4_inode_block_valid(struct inode *inode, ext4_fsblk_t start_blk,
 {
 	return ext4_sb_block_valid(inode->i_sb, inode, start_blk, count);
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_inode_block_valid);
 
 int ext4_check_blockref(const char *function, unsigned int line,
 			struct inode *inode, __le32 *p, unsigned int max)
@@ -367,4 +375,5 @@ int ext4_check_blockref(const char *function, unsigned int line,
 	}
 	return 0;
 }
+EXPORT_SYMBOL_FOR_EXT4_TEST(ext4_check_blockref);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`call_rcu`
- 外部函数：`container_of`
- 外部函数：`ext4_blocks_count`
- 外部函数：`ext4_error_inode`
- 外部函数：`ext4_inode_block_valid`
- 外部函数：`kfree`
- 外部函数：`kmem_cache_free`
- 外部函数：`le32_to_cpu`
- 外部函数：`rb_entry`
- 外部函数：`rbtree_postorder_for_each_entry_safe`
- 外部函数：`rcu_dereference`
- 外部函数：`rcu_read_lock`
- 外部函数：`rcu_read_unlock`
- 外部函数：`unlikely`
- 大写宏：`EFSCORRUPTED`
- 大写宏：`EXT4_SB`
- 大写宏：`NULL`
- 外部类型：`Called`
- 外部类型：`Lock`
- 外部类型：`Returns`
- 外部类型：`ext4_fsblk_t`
- 外部类型：`ext4_sb_info`
- 外部类型：`ext4_system_blocks`
- 外部类型：`ext4_system_zone`
- 外部类型：`inode`
- 外部类型：`journal_t`
- 外部类型：`rb_node`
- 外部类型：`rcu_head`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-8f73510a8e` → 本草稿移入 `cases/defect/auto-linux-8f73510a8e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
